/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Scene.h"
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QDir>
#include <QTime>
#include "material/Diffuse.h"
#include "material/DiffuseEmitter.h"
#include "material/Glass.h"
#include "material/Hole.h"
#include "material/Mirror.h"
#include "material/Texture.h"
#include "material/ParticipatingMedium.h"
#include "geometry_instance/AABInstance.h"
#include "config.h"
#include "logging/DummyLogger.h"
#include <cstdio>
#include <optixu_matrix_namespace.h>
#include <sstream>
#include "util/RelPath.h"

Scene::Scene(Logger *logger)
    : m_scene(NULL),
      m_importer(new Assimp::Importer()),
      m_numTriangles(0),
      m_sceneFile(NULL),
	  m_logger(logger),
	  m_hasAnyHoleMaterial(false)
{

}

Scene::~Scene(void)
{
    // deleting m_importer also deletes the scene
    delete m_importer;
    for(int i = 0; i < m_materials.size(); i++)
    {
        delete m_materials.at(i);
    }
    m_materials.clear();
    delete m_sceneFile;
}

static optix::float3 toFloat3( aiVector3D vector)
{
    return optix::make_float3(vector.x, vector.y, vector.z);
}

static optix::float3 toFloat3( aiColor3D vector)
{
    return optix::make_float3(vector.r, vector.g, vector.b);
}

static void minCoordinates(Vector3 & min, const aiVector3D & vector)
{
    min.x = optix::fminf(min.x, vector.x);
    min.y = optix::fminf(min.y, vector.y);
    min.z = optix::fminf(min.z, vector.z);
}

static void maxCoordinates(Vector3 & max, const aiVector3D & vector)
{
    max.x = optix::fmaxf(max.x, vector.x);
    max.y = optix::fmaxf(max.y, vector.y);
    max.z = optix::fmaxf(max.z, vector.z);
}

Scene* Scene::createFromFile(const char* filename)
{
	return createFromFile(new DummyLogger(), filename);
}

Scene* Scene::createFromFile(Logger *logger, const char* filename )
{
    if(!QFile::exists(filename))
    {
        QString error = QString("The file that was supplied (%s) does not exist.").arg(filename);
        throw std::exception(error.toLatin1().constData());
    }

	QTime timerTotal;
	timerTotal.start();

	QScopedPointer<Scene> scenePtr (new Scene(logger));
    scenePtr->m_sceneFile = new QFileInfo(filename);

    // Remove point and lines from the model
    scenePtr->m_importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, 
        aiPrimitiveType_POINT | aiPrimitiveType_LINE );

	QTime readFileTimer;
	readFileTimer.start();

    scenePtr->m_scene = (aiScene *) scenePtr->m_importer->ReadFile( filename,
        aiProcess_Triangulate            |
        aiProcess_CalcTangentSpace       | 
        aiProcess_FindInvalidData        |
        aiProcess_GenUVCoords            |
        aiProcess_TransformUVCoords      |
        //aiProcess_FindInstances          |
        aiProcess_JoinIdenticalVertices  |
        //aiProcess_OptimizeGraph          | 
        aiProcess_OptimizeMeshes         |
        //aiProcess_PreTransformVertices   |
        aiProcess_GenSmoothNormals                              
        );

	logger->log("Scene createFromFile ReadFile: ellapsed %5.2fs\n", readFileTimer.elapsed() / 1000.0f);
        
    if(!scenePtr->m_scene)
    {
        QString error = QString("An error occurred in Assimp during reading of this file: %1").arg(scenePtr->m_importer->GetErrorString());
        throw std::exception(error.toUtf8().constData());
    }

	// gets the mapping from object id to node names and viceversa
	{
		QMap<unsigned int, aiNode *> objectIdToNode;
		unsigned int nObjects = 0;
		scenePtr->mapNodeObjectId(scenePtr->m_scene->mRootNode, nObjects, objectIdToNode);
		scenePtr->m_nodes.resize(nObjects);
		for(unsigned int objectId = 0; objectId < nObjects; ++objectId)
		{
			scenePtr->m_nodes[objectId] = objectIdToNode[objectId];
		}
	}

	normalizeMeshes(scenePtr->m_scene);

    // Load materials

    scenePtr->loadSceneMaterials();
    
    // Load lights from file

    scenePtr->loadLightSources();


	// walks to calculate area
	scenePtr->m_objectArea.resize(scenePtr->m_nodes.size());
	scenePtr->walkNode(scenePtr->m_scene, scenePtr->m_scene->mRootNode, 0);

	scenePtr->loadDiffuseEmmiters(scenePtr->m_scene->mRootNode);
	scenePtr->countTriangles();
	scenePtr->calcAABB();

    if(scenePtr->m_scene->mNumCameras > 0)
    {
        scenePtr->loadDefaultSceneCamera();
    }

    scenePtr->m_sceneName = QByteArray(scenePtr->m_sceneFile->absoluteFilePath().toLatin1().constData());

	logger->log("Scene createFromFile Total: ellapsed %5.2fs\n", timerTotal.elapsed() / 1000.0f);

    return scenePtr.take();
}

void Scene::loadSceneMaterials()
{
    //m_logger->log("NUM MATERIALS: %d\n", m_scene->mNumMaterials);
	m_hasAnyHoleMaterial = false;
    for(unsigned int i = 0; i < m_scene->mNumMaterials; i++)
    {
        aiMaterial* material = m_scene->mMaterials[i];
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        //m_logger->log("Material %d, %s:\n", i, name.C_Str());

        // Check if this is an Emitter
        aiColor3D emissivePower;
        if(material->Get(AI_MATKEY_COLOR_EMISSIVE, emissivePower) == AI_SUCCESS &&
            colorHasAnyComponent(emissivePower))
        {
            aiColor3D diffuseColor;
            if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) != AI_SUCCESS)
            {
                diffuseColor.r = 1;
                diffuseColor.g = 1;
                diffuseColor.b = 1;
            }
            Material* material = new DiffuseEmitter(toFloat3(emissivePower), toFloat3(diffuseColor));
            m_materials.push_back(material);
            continue;
        }

        // Textured material
        aiString textureName;
        if(material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName) == AI_SUCCESS)
        {
            QString textureAbsoluteFilePath = QString("%1/%2").arg(m_sceneFile->absoluteDir().absolutePath(), textureName.C_Str());
            
            // Use the displacement map as a normal map (in the crytek sponza test scene)
            aiString normalsName;
            Material* matl;
            if(material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normalsName) == AI_SUCCESS)
            {
                m_logger->log("Found normal map %s!\n", normalsName.C_Str());
                QString normalsAbsoluteFilePath = QString("%1/%2").arg(m_sceneFile->absoluteDir().absolutePath(), normalsName.C_Str());
                matl = new Texture(textureAbsoluteFilePath, normalsAbsoluteFilePath);
            }
            else
            {
                matl = new Texture(textureAbsoluteFilePath);
            }
            
            m_materials.push_back(matl);
            continue;
        }

        // Glass Material

        float indexOfRefraction;
        if(material->Get(AI_MATKEY_REFRACTI, indexOfRefraction) == AI_SUCCESS && indexOfRefraction > 1.0f)
        {
            //m_logger->log("\tGlass: IOR: %g\n", indexOfRefraction);
            Material* material = new Glass(indexOfRefraction, optix::make_float3(1,1,1));
            m_materials.push_back(material);
            continue;
        }

		// Hole Material
		if(material->Get(AI_MATKEY_REFRACTI, indexOfRefraction) == AI_SUCCESS && indexOfRefraction < 1.0f)
		{
			Material* material = new Hole();
            m_materials.push_back(material);
			m_hasAnyHoleMaterial = true;
            continue;
		}

        // Reflective/mirror material
        aiColor3D reflectiveColor;
        if(material->Get(AI_MATKEY_COLOR_REFLECTIVE, reflectiveColor) == AI_SUCCESS
            && colorHasAnyComponent(reflectiveColor))
        {
            //m_logger->log("\tReflective color: %.2f %.2f %.2f\n", reflectiveColor.r, reflectiveColor.g, reflectiveColor.b);
            Material* material = new Mirror(toFloat3(reflectiveColor));
            m_materials.push_back(material);
            continue;
        }

        // Diffuse

        aiColor3D diffuseColor;
        if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
        {
            //m_logger->log("\tDiffuse %.2f %.2f %.2f\n", diffuseColor.r, diffuseColor.g, diffuseColor.b);
            Material* material = new Diffuse(toFloat3(diffuseColor));
            m_materials.push_back(material);
            continue;
        }

        // Fall back to a red diffuse material

        m_logger->log("\tError: Found no material instance to create for material index: %d\n", i);
        m_materials.push_back(new Diffuse(optix::make_float3(1,0,0)));
    }
}

void Scene::loadLightSources()
{
    for(unsigned int i = 0; i < m_scene->mNumLights; i++)
    {
        aiLight* lightPtr = m_scene->mLights[i];

		aiNode *lightNode = m_scene->mRootNode->FindNode(lightPtr->mName);
		aiMatrix4x4 transformation;
		aiMatrix4x4 centeredTransformation;
		if(lightNode != NULL)
		{
			transformation = getTransformation(lightNode);
			centeredTransformation = getCenteredMatrix(transformation);
		}
		
        if(lightPtr->mType == aiLightSource_POINT)
        {
			Light light = Light::createPoint(lightPtr->mName.C_Str(), toFloat3(lightPtr->mColorDiffuse), toFloat3(transformation * lightPtr->mPosition));
            m_lights.push_back(light);
        }
        else if(lightPtr->mType == aiLightSource_SPOT)
        {
			Light light = Light::createSpot (
				lightPtr->mName.C_Str(),
				toFloat3(lightPtr->mColorDiffuse),
				toFloat3(transformation * lightPtr->mPosition),
				toFloat3(centeredTransformation * lightPtr->mDirection),
				lightPtr->mAngleInnerCone);
            m_lights.push_back(light);
        }
		else if(lightPtr->mType == aiLightSource_DIRECTIONAL)
		{
			Light light = Light::createDirectional(
				lightPtr->mName.C_Str(),
				toFloat3(lightPtr->mColorDiffuse),
				toFloat3(centeredTransformation * lightPtr->mDirection)
				);
			m_lights.push_back(light);
		}
    }
}

void Scene::loadMeshLightSource(const aiNode *node, aiMesh* mesh, DiffuseEmitter* diffuseEmitter )
{
    // Convert mesh into a quad light source

    if(mesh->mNumFaces < 1 || mesh->mNumFaces > 2)
    {
        aiString name;
        m_scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, name);
        m_logger->log("Material %s: Does only support quadrangle light source NumFaces: %d.\n", name.C_Str(), mesh->mNumFaces);
    }

    aiFace face = mesh->mFaces[0];
    if(face.mNumIndices == 3)
    {
        optix::float3 anchor = toFloat3(mesh->mVertices[face.mIndices[0]]);
        optix::float3 p1 = toFloat3(mesh->mVertices[face.mIndices[1]]);
        optix::float3 p2 = toFloat3(mesh->mVertices[face.mIndices[2]]);
        optix::float3 v1 = p1-anchor;
        optix::float3 v2 = p2-anchor;

		Light light = Light::createParalelogram(node->mName.C_Str(), diffuseEmitter->getPower(), anchor, v1, v2);
        m_lights.push_back(light);
        diffuseEmitter->setInverseArea(light.inverseArea);
    }
}

optix::Group Scene::getSceneRootGroup( optix::Context & context, QMap<QString, optix::Group> *nameMapping)
{
    if(!m_intersectionProgram)
    {
        std::string ptxFilename = relativePathToExe("TriangleMesh.cu.ptx");
        m_intersectionProgram = context->createProgramFromPTXFile( ptxFilename, "mesh_intersect" );
        m_boundingBoxProgram = context->createProgramFromPTXFile( ptxFilename, "mesh_bounds" );
    }

	QTime timer;
	timer.start();


    // Convert meshes into Geometry objects
    QVector<optix::Geometry> geometries;
    for(unsigned int i = 0; i < m_scene->mNumMeshes; i++)
    {
        optix::Geometry geometry = createGeometryFromMesh(m_scene->mMeshes[i], context);
        geometries.push_back(geometry);
    }

    // Convert nodes into a full scene Group
	optix::Group rootNodeGroup = getGroupFromNode(context, m_scene->mRootNode, geometries, m_materials, nameMapping);

#if ENABLE_PARTICIPATING_MEDIA
    {
        ParticipatingMedium partmedium = ParticipatingMedium(0.05, 0.01);
        AAB box = m_sceneAABB;
        box.addPadding(0.01);
        AABInstance participatingMediumCube (partmedium, box);
        optix::GeometryGroup group = context->createGeometryGroup();
        group->setChildCount(1);
        group->setChild(0, participatingMediumCube.getOptixGeometryInstance(context));
        optix::Acceleration a = context->createAcceleration("NoAccel", "NoAccel");
        group->setAcceleration(a);
        rootNodeGroup->setChildCount(rootNodeGroup->getChildCount()+1);
        rootNodeGroup->setChild(rootNodeGroup->getChildCount()-1, group);
    }
#endif

    optix::Acceleration acceleration = context->createAcceleration("Trbvh", "Bvh");
    rootNodeGroup->setAcceleration( acceleration );
    acceleration->markDirty();

	m_logger->log("Scene getSceneRootGroup: ellapsed %5.2fs\n", timer.elapsed() / 1000.0f);
    return rootNodeGroup;
}

optix::Geometry Scene::createGeometryFromMesh(aiMesh* mesh, optix::Context & context)
{
    unsigned int numFaces = mesh->mNumFaces;
    unsigned int numVertices = mesh->mNumVertices;
	

    optix::Geometry geometry = context->createGeometry();
    geometry->setPrimitiveCount(numFaces);
    geometry->setIntersectionProgram(m_intersectionProgram);
    geometry->setBoundingBoxProgram(m_boundingBoxProgram);

    // Create vertex, normal and texture buffer

    optix::Buffer vertexBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, numVertices);
    optix::float3* vertexBuffer_Host = static_cast<optix::float3*>( vertexBuffer->map() );

    optix::Buffer normalBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, numVertices);
    optix::float3* normalBuffer_Host = static_cast<optix::float3*>( normalBuffer->map() );

    geometry["vertexBuffer"]->setBuffer(vertexBuffer);
    geometry["normalBuffer"]->setBuffer(normalBuffer);

    // Copy vertex and normal buffers

    memcpy( static_cast<void*>( vertexBuffer_Host ),
        static_cast<void*>( mesh->mVertices ),
        sizeof( optix::float3 )*numVertices); 
    vertexBuffer->unmap();

    memcpy( static_cast<void*>( normalBuffer_Host ),
        static_cast<void*>( mesh->mNormals),
        sizeof( optix::float3 )*numVertices); 
    normalBuffer->unmap();

    // Transfer texture coordinates to buffer
    optix::Buffer texCoordBuffer;
    if(mesh->HasTextureCoords(0))
    {
        texCoordBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, numVertices);
        optix::float2* texCoordBuffer_Host = static_cast<optix::float2*>( texCoordBuffer->map());
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D texCoord = (mesh->mTextureCoords[0])[i];
            texCoordBuffer_Host[i].x = texCoord.x;
            texCoordBuffer_Host[i].y = texCoord.y;
        }
        texCoordBuffer->unmap();
    }
    else
    {
        texCoordBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, 0);
    }

    geometry["texCoordBuffer"]->setBuffer(texCoordBuffer);

    // Tangents and bi-tangents buffers

    geometry["hasTangentsAndBitangents"]->setUint(mesh->HasTangentsAndBitangents() ? 1 : 0);
    if(mesh->HasTangentsAndBitangents())
    {
        optix::Buffer tangentBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, numVertices);
        optix::float3* tangentBuffer_Host = static_cast<optix::float3*>( tangentBuffer->map() );
        memcpy( static_cast<void*>( tangentBuffer_Host ),
            static_cast<void*>( mesh->mTangents),
            sizeof( optix::float3 )*numVertices); 
        tangentBuffer->unmap();

        optix::Buffer bitangentBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, numVertices);
        optix::float3* bitangentBuffer_Host = static_cast<optix::float3*>( bitangentBuffer->map() );
        memcpy( static_cast<void*>( bitangentBuffer_Host ),
            static_cast<void*>( mesh->mBitangents),
            sizeof( optix::float3 )*numVertices); 
        bitangentBuffer->unmap();

        geometry["tangentBuffer"]->setBuffer(tangentBuffer);
        geometry["bitangentBuffer"]->setBuffer(bitangentBuffer);
    }
    else
    {
        optix::Buffer emptyBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT3, 0);
        geometry["tangentBuffer"]->setBuffer(emptyBuffer);
        geometry["bitangentBuffer"]->setBuffer(emptyBuffer);
    }

    // Create index buffer

    optix::Buffer indexBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT3, numFaces );
    optix::int3* indexBuffer_Host = static_cast<optix::int3*>( indexBuffer->map() );
    geometry["indexBuffer"]->setBuffer(indexBuffer);

    // Copy index buffer from host to device

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        indexBuffer_Host[i].x = face.mIndices[0];
        indexBuffer_Host[i].y = face.mIndices[1];
        indexBuffer_Host[i].z = face.mIndices[2];
    }

    indexBuffer->unmap();
    return geometry;

}

void Scene::loadDefaultSceneCamera()
{
    aiNode* cameraNode = m_scene->mRootNode->FindNode(m_scene->mCameras[0]->mName);
    aiCamera* camera = m_scene->mCameras[0];

	aiMatrix4x4 cameraTransformation = getTransformation(cameraNode);
	aiMatrix4x4 centeredCameraTransformation = getCenteredMatrix(cameraTransformation);

    aiVector3D eye = cameraTransformation * camera->mPosition;
    aiVector3D lookAt = eye + centeredCameraTransformation * camera->mLookAt;
    aiVector3D up = centeredCameraTransformation * camera->mUp;

    m_defaultCamera = Camera(Vector3(eye.x, eye.y, eye.z),
        Vector3(lookAt.x, lookAt.y, lookAt.z),
        Vector3(optix::normalize(optix::make_float3(up.x, up.y, up.z))),
        camera->mHorizontalFOV*365.0f/(2.0f*M_PIf),
        camera->mHorizontalFOV*365.0f/(2.0f*M_PIf),
        0.f,
        Camera::KeepHorizontal );
}

optix::Group Scene::getGroupFromNode(optix::Context & context, aiNode* node, QVector<optix::Geometry> & geometries, QVector<Material*> & materials, QMap<QString, optix::Group> *nameMapping)
{
	if (QString(node->mName.C_Str()).toLower().endsWith("__geometry") || !node->mNumMeshes && !node->mNumChildren)
	{
		optix::Group emptyGroup = context->createGroup();
		optix::Acceleration acceleration = context->createAcceleration("NoAccel", "NoAccel");
		emptyGroup->setAcceleration(acceleration);

		if (nameMapping != NULL)
		{
			nameMapping->insert(node->mName.C_Str(), emptyGroup);
		}

		return emptyGroup;
	} 
	else if(node->mNumMeshes > 0)
    {
        QVector<optix::GeometryInstance> instances;
        optix::GeometryGroup geometryGroup = context->createGeometryGroup();
        geometryGroup->setChildCount(node->mNumMeshes);

        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            unsigned int meshIndex = node->mMeshes[i];
            aiMesh* mesh = m_scene->mMeshes[meshIndex];
            unsigned int materialIndex = mesh->mMaterialIndex;
			Material* geometryMaterial = materials.at(materialIndex)->clone();
			geometryMaterial->setObjectId(m_nodeToId[node]);
			optix::GeometryInstance instance = getGeometryInstance(context, geometries[meshIndex], geometryMaterial);
            geometryGroup->setChild(i, instance);
			
            if(dynamic_cast<DiffuseEmitter*>(geometryMaterial) != NULL)
            {
                DiffuseEmitter* emitterMaterial = (DiffuseEmitter*)(geometryMaterial);
				loadMeshLightSource(node, mesh, emitterMaterial);
            }

			delete geometryMaterial;
        }

        {
            optix::Acceleration acceleration = context->createAcceleration("Trbvh", "Bvh");
            acceleration->setProperty( "vertex_buffer_name", "vertexBuffer" );
            acceleration->setProperty( "index_buffer_name", "indexBuffer" );
            geometryGroup->setAcceleration( acceleration );
            acceleration->markDirty();
        }

        // Create group that contains the GeometryInstance
		optix::Transform transform = context->createTransform();
		transform->setMatrix(false, optix::Matrix4x4::identity().getData(), NULL); 
		transform->setChild(geometryGroup);

        optix::Group group = context->createGroup();
        group->setChildCount(1);
		group->setChild(0, transform);
        {
            optix::Acceleration acceleration = context->createAcceleration("NoAccel", "NoAccel");
            group->setAcceleration( acceleration );
        }

		if(nameMapping != NULL)
		{
			nameMapping->insert(node->mName.C_Str(), group);
		}

        return group;
    }
    else if(node->mNumChildren > 0)
    {
        QVector<optix::Transform> transforms;
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            aiNode* childNode = node->mChildren[i];
			optix::Group childGroup = getGroupFromNode(context, childNode, geometries, materials, nameMapping);
            if(childGroup)
            {
				auto childTransform = context->createTransform();
				childTransform->setMatrix(false, optix::Matrix4x4::identity().getData(), NULL); 
				childTransform->setChild(childGroup);
                transforms.push_back(childTransform);
            }
        }

        optix::Group group = context->createGroup(transforms.begin(), transforms.end());
        optix::Acceleration acceleration = context->createAcceleration("Trbvh", "Bvh");
        group->setAcceleration( acceleration );

		if(nameMapping != NULL)
		{
			nameMapping->insert(node->mName.C_Str(), group);
		}

        return group;
    }
	else
	{
		throw std::logic_error("Node should have nodes or childen, or be empty");
	}
}

optix::GeometryInstance Scene::getGeometryInstance( optix::Context & context, optix::Geometry & geometry, Material* material )
{
	optix::Material optix_material = material->getOptixMaterial(context, m_hasAnyHoleMaterial);
    optix::GeometryInstance instance = context->createGeometryInstance( geometry, &optix_material, &optix_material+1 );
    material->registerInstanceValues(instance);
    return instance;
}

bool Scene::colorHasAnyComponent(const aiColor3D & color)
{
    return color.r > 0 && color.g > 0 && color.b > 0;
}

const QVector<Light> & Scene::getSceneLights() const
{
    return m_lights;
}

Camera Scene::getDefaultCamera() const
{
    return m_defaultCamera;
}

const char* Scene::getSceneName() const
{
    return m_sceneName.constData();
}

unsigned int Scene::getNumTriangles() const
{
    return m_numTriangles;
}

AAB Scene::getSceneAABB() const
{
    return m_sceneAABB;
}

void Scene::walkNode(const aiScene *scene, const aiNode *node, int depth)
{
	if(!node)
		return;
	for(int i = 0; i < depth; ++i)
	{
		m_logger->log(" ");
	}
	
	float area = getNodeArea(scene, node);

	m_logger->log("%s: area %f\n", node->mName.C_Str(), area);

	unsigned int nodeId = m_nodeToId.value((aiNode *) node, UINT_MAX);
	if(nodeId != UINT_MAX)
	{
		m_objectArea[nodeId] = area;
	}

	for(unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		walkNode(scene, node->mChildren[i], depth+1);
	}
}

float Scene::getNodeArea(const aiScene *scene, const aiNode *node)
{
	double area = 0;

	for(unsigned int meshIdx = 0; meshIdx < node->mNumMeshes; ++meshIdx)
	{
		auto mesh = m_scene->mMeshes[node->mMeshes[meshIdx]];
		for(unsigned faceIdx = 0; faceIdx < mesh->mNumFaces; ++ faceIdx)
		{
			auto face = mesh->mFaces[faceIdx];
			if (face.mNumIndices != 3){
				std::stringstream ss;
				ss << "Non-triangle face in mesh " << node->mName.C_Str() << ". Got " << face.mNumIndices << " vertex(es).";
				throw std::invalid_argument(ss.str().c_str());
			}
			
			auto A = mesh->mVertices[face.mIndices[0]];
			auto B = mesh->mVertices[face.mIndices[1]];
			auto C = mesh->mVertices[face.mIndices[2]];

			float c = (B - A).Length();
			float b = (C - A).Length();
			float a = (C - B).Length();

			float det = 2*b*b*c*c + 2*c*c*a*a + 2*a*a*b*b - a*a*a*a - b*b*b*b - c*c*c*c;
			if(det >= 0.0)
			{
				area += 0.25 * sqrt(det);
			}
		}
	}

	return area;
}

float Scene::getSceneInitialPPMRadiusEstimate() const
{
    Vector3 sceneExtent = getSceneAABB().getExtent();
    float volume = sceneExtent.x*sceneExtent.y*sceneExtent.z;
    float cubelength = pow(volume, 1.f/3.f);
    float A = 6*cubelength*cubelength;
    float radius = A*0.0004f;
    return radius;
}

void Scene::normalizeMeshes(aiScene *scene)
{
	normalizeMeshes(scene, aiMatrix4x4(), scene->mRootNode);
}

void Scene::normalizeMeshes(aiScene *scene, aiMatrix4x4 transformation, aiNode *node)
{
	if(node == NULL)
		return;

	transformation = transformation * node->mTransformation;
	
	for(unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		normalizeMesh(scene->mMeshes[node->mMeshes[i]], transformation);
	}

	for(unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		normalizeMeshes(scene, transformation, node->mChildren[i]);
	}
}

void Scene::normalizeMesh(aiMesh *mesh, aiMatrix4x4 transformation)
{
	aiMatrix4x4 centeredTransformation = getCenteredMatrix(transformation);

	for(unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		mesh->mVertices[i] = transformation * mesh->mVertices[i];
	}
	
	if(mesh->HasNormals())
	{
		for(unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			mesh->mNormals[i] = centeredTransformation * mesh->mNormals[i];
			if(mesh->mTangents != NULL)
			{
				mesh->mTangents[i] = centeredTransformation * mesh->mTangents[i];
				mesh->mBitangents[i] = centeredTransformation * mesh->mBitangents[i];
			}
		}
	}
}

aiMatrix4x4 Scene::getTransformation(aiNode *node)
{
	if(node == NULL)
		return aiMatrix4x4();
	else
		return getTransformation(node->mParent) * node->mTransformation;
}

aiMatrix4x4 Scene::getCenteredMatrix(const aiMatrix4x4 &matrix)
{
	aiMatrix4x4 res(matrix);
	res.a4 = res.b4 = res.c4 = 0.0f;
	return res;
}

void Scene::printMatrix(const aiMatrix4x4& matrix)
{
	m_logger->log("%5.2f %5.2f %5.2f %5.2f\n%5.2f %5.2f %5.2f %5.2f\n%5.2f %5.2f %5.2f %5.2f\n%5.2f %5.2f %5.2f %5.2f\n",
		matrix.a1, matrix.a2, matrix.a3, matrix.a4, 
		matrix.b1, matrix.b2, matrix.b3, matrix.b4, 
		matrix.c1, matrix.c2, matrix.c3, matrix.c4, 
		matrix.d1, matrix.d2, matrix.d3, matrix.d4);
}

void Scene::mapNodeObjectId(aiNode *node, unsigned int& objectIdCumulative, QMap<unsigned int, aiNode *> &objectIdToNode)
{
	if(node == NULL)
		return;
	
	m_nodeToId[node] = objectIdCumulative;
	m_nameToId[node->mName.C_Str()] = objectIdCumulative;
	objectIdToNode[objectIdCumulative] = node;
	++objectIdCumulative;
	

	for(unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		mapNodeObjectId(node->mChildren[i], objectIdCumulative, objectIdToNode);
	}
}


float Scene::getObjectArea(int objectId) const
{
	return m_objectArea.at(objectId);
}

QVector<Vector3> Scene::getObjectPoints(int objectId) const
{
	QVector<Vector3> res;
	auto node = m_nodes.at(objectId);
	if(node == NULL)
	{
		throw std::invalid_argument("Scene::getObjectName: No such object id");
	}
	for(unsigned int meshId = 0; meshId < node->mNumMeshes; ++meshId)
	{
		auto mesh = m_scene->mMeshes[node->mMeshes[meshId]];
		
		for(unsigned int vertex = 0; vertex < mesh->mNumVertices; ++vertex)
		{
			auto aiVertex = mesh->mVertices[vertex];
			res.append(Vector3(aiVertex.x, aiVertex.y, aiVertex.z));
		}
	}

	return res;
}

unsigned int Scene::getObjectId(const QString& objectName) const
{
	return m_nameToId.value(objectName, UINT_MAX);
}

QString Scene::getObjectName(int objectId) const
{
	auto node = m_nodes.at(objectId);
	if(node == NULL)
	{
		throw std::invalid_argument("Scene::getObjectName: No such object id");
	}
	return node->mName.C_Str();
}

void Scene::calcAABB()
{
	Vector3 sceneAABBMin (1e33f);
    Vector3 sceneAABBMax (-1e33f);
	// Find scene AABB
    for(unsigned int i = 0; i < m_scene->mNumMeshes; i++)
    {
        aiMesh* mesh = m_scene->mMeshes[i];

        m_numTriangles += mesh->mNumFaces;

        for(unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            aiVector3D p1 = mesh->mVertices[face.mIndices[0]];
            aiVector3D p2 = mesh->mVertices[face.mIndices[0]];
            aiVector3D p3 = mesh->mVertices[face.mIndices[0]];
            minCoordinates(sceneAABBMin, p1);
            minCoordinates(sceneAABBMin, p2);
            minCoordinates(sceneAABBMin, p3);
            maxCoordinates(sceneAABBMax, p1);
            maxCoordinates(sceneAABBMax, p2);
            maxCoordinates(sceneAABBMax, p3);
        }
	}

    m_sceneAABB.min = sceneAABBMin;
    m_sceneAABB.max = sceneAABBMax;
}

void Scene::countTriangles()
{
	m_numTriangles = 0;
	for(unsigned int i = 0; i < m_scene->mNumMeshes; i++)
    {
        aiMesh* mesh = m_scene->mMeshes[i];
        m_numTriangles += mesh->mNumFaces;
    }
}

void Scene::loadDiffuseEmmiters(const aiNode *node)
{
	if(!node)
		return;

	for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
		aiMesh* mesh = m_scene->mMeshes[node->mMeshes[i]];

        // Check if this is a diffuse emitter
        unsigned int materialIndex = mesh->mMaterialIndex;
        Material* geometryMaterial = m_materials.at(materialIndex);
        if(dynamic_cast<DiffuseEmitter*>(geometryMaterial) != NULL)
        {
            DiffuseEmitter* emitterMaterial = (DiffuseEmitter*)(geometryMaterial);
            loadMeshLightSource(node, mesh, emitterMaterial);
        }
    }

	for(unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		loadDiffuseEmmiters(node->mChildren[i]);
	}
}

QVector<QString> Scene::getObjectIdToNameMap() const
{
	QVector<QString> res;
	for(auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		res.append((*it)->mName.C_Str());
	}
	return res;
}
/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include "renderer/Light.h"
#include "render_engine_export_api.h"
#include <QVector>
#include <QByteArray>
#include <QMap>
#include <optixu/optixpp_namespace.h>
#include "renderer/Camera.h"
#include "renderer/Light.h"
#include "render_engine_export_api.h"
#include "math/AAB.h"
#include "math/Vector3.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>

class DiffuseEmitter;
class QFileInfo;
class Logger;
class Material;


class Scene
{
public:
    RENDER_ENGINE_EXPORT_API virtual ~Scene(void);
    RENDER_ENGINE_EXPORT_API static Scene* createFromFile(const char* file);
	RENDER_ENGINE_EXPORT_API static Scene* createFromFile(Logger *logger, const char* file);
	virtual optix::Group getSceneRootGroup(optix::Context & context, QMap<QString, optix::Group> *nameMapping = NULL);
    void loadDefaultSceneCamera();
    virtual const QVector<Light> & getSceneLights() const;
    virtual Camera getDefaultCamera() const;
    virtual const char* getSceneName() const;
    virtual AAB getSceneAABB() const ;
    RENDER_ENGINE_EXPORT_API virtual unsigned int getNumTriangles() const;
	RENDER_ENGINE_EXPORT_API float getSceneInitialPPMRadiusEstimate() const;
	RENDER_ENGINE_EXPORT_API QVector<QString> getObjectIdToNameMap() const;


	// Scene object information
	RENDER_ENGINE_EXPORT_API float getObjectArea(int objectId) const;
	RENDER_ENGINE_EXPORT_API QVector<Vector3> getObjectPoints(int objectId) const;
	RENDER_ENGINE_EXPORT_API unsigned int getObjectId(const QString& objectName) const;
	RENDER_ENGINE_EXPORT_API QString getObjectName(int objectId) const;

private:
	Scene(Logger *logger);
    optix::Geometry Scene::createGeometryFromMesh(aiMesh* mesh, optix::Context & context);
	void loadMeshLightSource(const aiNode *node, aiMesh* mesh, DiffuseEmitter* diffuseEmitter );
    optix::Group getGroupFromNode(optix::Context & context, aiNode* node, QVector<optix::Geometry> & geometries, QVector<Material*> & materials, QMap<QString, optix::Group> *nameMapping);
    optix::GeometryInstance getGeometryInstance( optix::Context & context, optix::Geometry & geometry, Material* material );
    bool colorHasAnyComponent(const aiColor3D & color);
    void loadSceneMaterials();
    void loadLightSources();
	void walkNode(const aiScene *scene, const aiNode *node, int depth);
	void mapNodeObjectId(aiNode *node, unsigned int& objectIdCumulative, QMap<unsigned int, aiNode *> &objectIdToNode);
	aiMatrix4x4 getTransformation(aiNode *node);
	void printMatrix(const aiMatrix4x4 &matrix);
	float getNodeArea(const aiScene *scene, const aiNode *node);
	static aiMatrix4x4 getCenteredMatrix(const aiMatrix4x4 &matrix);
	static void normalizeMeshes(aiScene *scene);
	static void normalizeMeshes(aiScene *scene, aiMatrix4x4 transformation, aiNode *node);
	static void normalizeMesh(aiMesh *mesh, aiMatrix4x4 transformation);
	void countTriangles();
	void calcAABB();
	void loadDiffuseEmmiters(const aiNode *fromNode);

    QVector<Material*> m_materials;
    QVector<Light> m_lights;
    QByteArray m_sceneName;
    QFileInfo* m_sceneFile; 
    Assimp::Importer* m_importer;
    aiScene* m_scene;
    optix::Program m_intersectionProgram;
    optix::Program m_boundingBoxProgram;
    Camera m_defaultCamera;
    AAB m_sceneAABB;
    unsigned int m_numTriangles;
	Logger *m_logger;

	// mappings to retrieve object info
	QMap<aiNode *, unsigned int> m_nodeToId; /// internal, for geometry assignment
	QMap<QString, unsigned int> m_nameToId;
	QVector<aiNode *> m_nodes;
	QVector<float> m_objectArea;
};
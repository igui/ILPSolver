/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <cuda.h>
#include <curand_kernel.h>
#include "PMOptixRenderer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <limits>
#include "config.h"
#include "RandomState.h"
#include "renderer/OptixEntryPoint.h"
#include "renderer/Hitpoint.h"
#include "renderer/ppm/Photon.h"
#include "Camera.h"
#include <QThread>
#include <QMap>
#include <sstream>
#include "renderer/RayType.h"
#include "ComputeDevice.h"
#include "clientserver/RenderServerRenderRequest.h"
#include <exception>
#include "util/sutil.h"
#include "scene/Scene.h"
#include "renderer/helpers/nsight.h"
#include "util/RelPath.h"

const unsigned int PMOptixRenderer::PHOTON_GRID_MAX_SIZE = 100*100*100;
const unsigned int PMOptixRenderer::MAX_PHOTON_COUNT = MAX_PHOTONS_DEPOSITS_PER_EMITTED;
using namespace optix;


PMOptixRenderer::PMOptixRenderer() : 
    m_initialized(false),
    m_width(10),
    m_height(10),
	m_photonWidth(10),
	m_groups(new QMap<QString, Group>()),
	m_lights(new QMap<QString, QList<int>>())
{
    try
    {
        m_context = optix::Context::create();
        if(!m_context)
        {
            throw std::exception("Unable to create OptiX context.");
        }
    }
    catch(const optix::Exception & e)
    {
        throw std::exception(e.getErrorString().c_str());
    }
    catch(const std::exception & e)
    {
        QString error = QString("Error during initialization of Optix: %1").arg(e.what());
        throw std::exception(error.toLatin1().constData());
    }
}

PMOptixRenderer::~PMOptixRenderer()
{
    m_context->destroy();
    cudaDeviceReset();
}


void PMOptixRenderer::initialize(const ComputeDevice & device, Logger *logger)
{
    if(m_initialized)
    {
        throw std::exception("ERROR: Multiple PMOptixRenderer::initialize!\n");
    }
	m_logger = logger;

    initDevice(device);

    m_context->setRayTypeCount(RayType::NUM_RAY_TYPES);
    m_context->setEntryPointCount(OptixEntryPoint::NUM_PASSES-1);
    m_context->setStackSize(1596);
	m_context->setPrintEnabled(true);

    m_context["maxPhotonDepositsPerEmitted"]->setUint(MAX_PHOTON_COUNT);
    m_context["ppmRadius"]->setFloat(0.f);
    m_context["ppmRadiusSquared"]->setFloat(0.f);
    m_context["emittedPhotonsPerIterationFloat"]->setFloat(0.f);
    m_context["photonLaunchWidth"]->setUint(0);
	m_context["storefirstHitPhotons"]->setUint(0);

    // An empty scene root node
    optix::Group group = m_context->createGroup();
    m_context["sceneRootObject"]->set(group);
    
    // Display buffer

    // Ray Trace OptixEntryPoint Output Buffer

    m_raytracePassOutputBuffer = m_context->createBuffer( RT_BUFFER_INPUT_OUTPUT );
    m_raytracePassOutputBuffer->setFormat( RT_FORMAT_USER );
    m_raytracePassOutputBuffer->setElementSize( sizeof( Hitpoint ) );
    m_raytracePassOutputBuffer->setSize( m_width, m_height );
    m_context["raytracePassOutputBuffer"]->set( m_raytracePassOutputBuffer );


    // Ray OptixEntryPoint Generation Program

    {
		Program generatorProgram = createProgram("PMRayGenerator.cu.ptx", "generateRay" );
        Program exceptionProgram = createProgram("PMRayGenerator.cu.ptx", "exception" );
        Program missProgram = createProgram("PMRayGenerator.cu.ptx", "miss" );
        
        m_context->setRayGenerationProgram( OptixEntryPoint::PPM_RAYTRACE_PASS, generatorProgram );
        m_context->setExceptionProgram( OptixEntryPoint::PPM_RAYTRACE_PASS, exceptionProgram );
        m_context->setMissProgram(RayType::RADIANCE, missProgram);
        m_context->setMissProgram(RayType::RADIANCE_IN_PARTICIPATING_MEDIUM, missProgram);
    }

    //
    // Photon Tracing OptixEntryPoint
    //

    {
        Program generatorProgram = createProgram("PMPhotonGenerator.cu.ptx", "generator" );
        Program exceptionProgram = createProgram("PMPhotonGenerator.cu.ptx", "exception" );
        Program missProgram = createProgram("PMPhotonGenerator.cu.ptx", "miss");
        m_context->setRayGenerationProgram(OptixEntryPoint::PPM_PHOTON_PASS, generatorProgram);
        m_context->setMissProgram(OptixEntryPoint::PPM_PHOTON_PASS, missProgram);
        m_context->setExceptionProgram(OptixEntryPoint::PPM_PHOTON_PASS, exceptionProgram);
    }

    m_photons = m_context->createBuffer(RT_BUFFER_OUTPUT);
    m_photons->setFormat( RT_FORMAT_USER );
    m_photons->setElementSize( sizeof( Photon ) );
	m_photons->setSize(getNumPhotons());
    m_context["photons"]->set( m_photons );
	m_context["photonsSize"]->setUint(getNumPhotons());

	m_powerEmittedBuffer = m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT);
	m_powerEmittedBuffer->setFormat(RT_FORMAT_FLOAT);
	m_powerEmittedBuffer->setSize(1);
	m_context["powerEmitted"]->set(m_powerEmittedBuffer);

    m_context["photonsGridCellSize"]->setFloat(0.0f);
    m_context["photonsGridSize"]->setUint(0,0,0);
    m_context["photonsWorldOrigo"]->setFloat(make_float3(0));
    m_photonsHashCells = m_context->createBuffer(RT_BUFFER_OUTPUT);
    m_photonsHashCells->setFormat( RT_FORMAT_UNSIGNED_INT );
	m_photonsHashCells->setSize( getNumPhotons() );
    m_hashmapOffsetTable = m_context->createBuffer(RT_BUFFER_OUTPUT);
    m_hashmapOffsetTable->setFormat( RT_FORMAT_UNSIGNED_INT );
    m_hashmapOffsetTable->setSize( PHOTON_GRID_MAX_SIZE+1 );
    m_context["hashmapOffsetTable"]->set( m_hashmapOffsetTable );

	m_hitCountBuffer = m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT);
	m_hitCountBuffer->setFormat(RT_FORMAT_UNSIGNED_INT);
	m_hitCountBuffer->setSize(10);
    m_context["hitCount"]->set( m_hitCountBuffer );

	m_rawRadianceBuffer = m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT);
	m_rawRadianceBuffer->setFormat(RT_FORMAT_FLOAT);
	m_rawRadianceBuffer->setSize(10);
    m_context["rawRadiance"]->set( m_rawRadianceBuffer );


    //
    // Indirect Radiance Estimation Buffer
    //

    m_indirectRadianceBuffer = m_context->createBuffer( RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT3, m_width, m_height );
    m_context["indirectRadianceBuffer"]->set( m_indirectRadianceBuffer );
    
    //
    // Indirect Radiance Estimation Program
    //
    {
        Program program = createProgram("PMIndirectRadianceEstimation.cu.ptx", "kernel" );
        m_context->setRayGenerationProgram(OptixEntryPoint::PPM_INDIRECT_RADIANCE_ESTIMATION_PASS, program );
    }

    //
    // Direct Radiance Estimation Buffer
    //

    m_directRadianceBuffer = m_context->createBuffer( RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, m_width, m_height );
    m_context["directRadianceBuffer"]->set( m_directRadianceBuffer );

    //
    // Direct Radiance Estimation Program
    //
    {
        Program program = createProgram("PMDirectRadianceEstimation.cu.ptx", "kernel" );
        m_context->setRayGenerationProgram(OptixEntryPoint::PPM_DIRECT_RADIANCE_ESTIMATION_PASS, program );
    }

    //
    // Output Buffer
    //
    {
        m_outputBuffer = m_context->createBuffer( RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, m_width, m_height );
        m_context["outputBuffer"]->set(m_outputBuffer);
    }

    //
    // Output Program
    //
    {
        Program program = createProgram("PMOutput.cu.ptx", "kernel" );
        m_context->setRayGenerationProgram(OptixEntryPoint::PPM_OUTPUT_PASS, program );
    }

    //
    // Random state buffer (must be large enough to give states to both photons and image pixels)
    //
  
    m_randomStatesBuffer = m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT|RT_BUFFER_GPU_LOCAL);
    m_randomStatesBuffer->setFormat( RT_FORMAT_USER );
    m_randomStatesBuffer->setElementSize( sizeof( RandomState ) );
	m_randomStatesBuffer->setSize(m_photonWidth, m_photonWidth);
    m_context["randomStates"]->set(m_randomStatesBuffer);

    //
    // Light sources buffer
    //

    m_lightBuffer = m_context->createBuffer(RT_BUFFER_INPUT);
    m_lightBuffer->setFormat(RT_FORMAT_USER);
    m_lightBuffer->setElementSize(sizeof(Light));
    m_lightBuffer->setSize(1);
    m_context["lights"]->set( m_lightBuffer );

    //
    // Debug buffers
    //

    m_initialized = true;
}

void PMOptixRenderer::initDevice(const ComputeDevice & device)
{
    // Set OptiX device as given by ComputeDevice::getDeviceId (Cuda ordinal)

    unsigned int deviceCount = m_context->getDeviceCount();
    int deviceOptixOrdinal = -1;
    for(unsigned int index = 0; index < deviceCount; ++index)
    {
        int cudaDeviceOrdinal;
        if(RTresult code = rtDeviceGetAttribute(index, RT_DEVICE_ATTRIBUTE_CUDA_DEVICE_ORDINAL, sizeof(int), &cudaDeviceOrdinal))
            throw Exception::makeException(code, 0);

        if(cudaDeviceOrdinal == device.getDeviceId())
        {
            deviceOptixOrdinal = index;
        }
    }

    m_optixDeviceOrdinal = deviceOptixOrdinal;

    if(deviceOptixOrdinal >= 0)
    {
        m_context->setDevices(&deviceOptixOrdinal, &deviceOptixOrdinal+1);
    }
    else
    {
        throw std::exception("Did not find OptiX device Number for given device. OptiX may not support this device.");
    }
}

void PMOptixRenderer::initScene( Scene & scene )
{
    if(!m_initialized)
    {
        throw std::exception("Cannot initialize scene before PMOptixRenderer.");
    }

    const QVector<Light> & lights = scene.getSceneLights();
    if(lights.size() == 0)
    {
        throw std::exception("No lights exists in this scene.");
    }

    try
    {
		m_groups->clear();
		m_sceneRootGroup = scene.getSceneRootGroup(m_context, m_groups);

        m_context["sceneRootObject"]->set(m_sceneRootGroup);
        m_sceneAABB = scene.getSceneAABB();
        Sphere sceneBoundingSphere = m_sceneAABB.getBoundingSphere();
        m_context["sceneBoundingSphere"]->setUserData(sizeof(Sphere), &sceneBoundingSphere);

		const float ppmRadius = scene.getSceneInitialPPMRadiusEstimate();
		m_scenePPMRadius = ppmRadius;
		m_context["ppmRadius"]->setFloat(ppmRadius);
        m_context["ppmRadiusSquared"]->setFloat(ppmRadius * ppmRadius);

		auto objectIdToName = scene.getObjectIdToNameMap();
		m_sceneObjects = objectIdToName.size();
		m_objectIdToName.resize(m_sceneObjects, "");
		for(unsigned int i = 0; i < m_sceneObjects; ++i)
		{
			m_objectIdToName.at(i) = objectIdToName.at(i).toLocal8Bit().constData();
		}
		m_hitCountBuffer->setSize(m_sceneObjects);
		m_rawRadianceBuffer->setSize(m_sceneObjects);

		// Add the lights from the scene to the light buffer
        m_lightBuffer->setSize(lights.size());
        Light* lights_host = (Light*)m_lightBuffer->map();
        memcpy(lights_host, scene.getSceneLights().constData(), sizeof(Light)*lights.size());
        m_lightBuffer->unmap();

		m_lights->clear();
		for(int lightIdx = 0; lightIdx < lights.size(); ++lightIdx)
		{
			const char *lightName = lights[lightIdx].name;
			if(!m_lights->contains(lightName))
			{
				(*m_lights)[lightName] = QList<int>();
			}
			
			(*m_lights)[lightName].append(lightIdx);
		}

        compile();

    }
    catch(const optix::Exception & e)
    {
        QString error = QString("An OptiX error occurred when initializing scene: %1").arg(e.getErrorString().c_str());
        throw std::exception(error.toLatin1().constData());
    }
}

void PMOptixRenderer::compile()
{
    try
    {
        m_context->validate();
        m_context->compile();
    }
    catch(const Exception& e)
    {
        throw e;
    }
}

void PMOptixRenderer::renderNextIteration(unsigned long long iterationNumber, unsigned long long localIterationNumber, float PPMRadius, 
                                        const RenderServerRenderRequestDetails & details)
{
	render(512, details.getHeight(), details.getWidth(), details.getCamera(), true, false);
}

void PMOptixRenderer::genPhotonMap(unsigned int photonLaunchWidth)
{
	render(photonLaunchWidth, 10, 10, Camera(), false, true);
}

void PMOptixRenderer::render(unsigned int photonLaunchWidth, unsigned int height, unsigned int width, const Camera camera, bool generateOutput, bool storefirstHitPhotons)
{
	//m_logger->log("START\n");
    if(!m_initialized)
    {
        throw std::exception("Traced before PMOptixRenderer was initialized.");
    }

	nvtx::ScopedRange r("PMOptixRenderer::Trace");

	//m_logger->log("Used host memory: %1.1fMib\n", m_context->getUsedHostMemory() / (1024.0f * 1024.0f) );

    try
    {
		m_context["storefirstHitPhotons"]->setUint(storefirstHitPhotons);

        // If the width and height of the current render request has changed, we must resize buffers
		if(width != m_width || height != m_height || photonLaunchWidth != m_photonWidth)
        {
			this->resizeBuffers(width, height, photonLaunchWidth);
        }

        double traceStartTime = sutilCurrentTime();
        m_context["camera"]->setUserData( sizeof(Camera), &camera );

		int numSteps = generateOutput ? 7 : 3;

		auto powerEmittedPtr = (float *) m_powerEmittedBuffer->map();
		*powerEmittedPtr = 0;
		m_powerEmittedBuffer->unmap();

        //
        // Photon Tracing
        //
        {
			double start = sutilCurrentTime();
            nvtx::ScopedRange r( "OptixEntryPoint::PHOTON_PASS" );
            m_context->launch( OptixEntryPoint::PPM_PHOTON_PASS,
                static_cast<unsigned int>(m_photonWidth),
                static_cast<unsigned int>(m_photonWidth) );
			double time = sutilCurrentTime() - start;
			//m_logger->log("1/%d PHOTON_PASS time: %1.3fs\n", numSteps, time);
        }

        //
        // Create Photon Map
        //
        {
			double start = sutilCurrentTime();
            nvtx::ScopedRange r( "Creating photon map" );
			createUniformGridPhotonMap(m_scenePPMRadius);
			double time = sutilCurrentTime() - start;
			//m_logger->log("2/%d Creating photon map time: %1.3fs\n", numSteps, time);
        }


		//
        // Get hit count
        //
        {
			double start = sutilCurrentTime();
            nvtx::ScopedRange r( "Counting hit count" );
			countHitCountPerObject();
			double time = sutilCurrentTime() - start;
			//m_logger->log("3/%d Creating photon map time: %1.3fs\n", numSteps, time);
        }


        //
        // Transfer any data from the photon acceleration structure build to the GPU (trigger an empty launch)
        //
        {
            nvtx::ScopedRange r("Transfer photon map to GPU");
            m_context->launch(OptixEntryPoint::PPM_INDIRECT_RADIANCE_ESTIMATION_PASS,
                0, 0);
        }

        // Trace viewing rays
        if(generateOutput){
			double start = sutilCurrentTime();
            nvtx::ScopedRange r("OptixEntryPoint::RAYTRACE_PASS");
            m_context->launch( OptixEntryPoint::PPM_RAYTRACE_PASS,
                static_cast<unsigned int>(m_width),
                static_cast<unsigned int>(m_height) );
			double time = sutilCurrentTime() - start;
			//m_logger->log("4/%d RAYTRACE_PASS time: %1.3fs\n", numSteps, time);
        }
    
        //
        // PPM Indirect Estimation (using the photon map)
        //
		if(generateOutput && !storefirstHitPhotons){
			double start = sutilCurrentTime();
            nvtx::ScopedRange r("OptixEntryPoint::INDIRECT_RADIANCE_ESTIMATION");
            m_context->launch(OptixEntryPoint::PPM_INDIRECT_RADIANCE_ESTIMATION_PASS,
                m_width, m_height);
			double time = sutilCurrentTime() - start;
			//m_logger->log("5/%d INDIRECT_RADIANCE_ESTIMATION time: %1.3fs\n", numSteps, time);
        }

        //
        // Direct Radiance Estimation
        //
        if(generateOutput){
			double start = sutilCurrentTime();
            nvtx::ScopedRange r("OptixEntryPoint::PPM_DIRECT_RADIANCE_ESTIMATION_PASS");
            m_context->launch(OptixEntryPoint::PPM_DIRECT_RADIANCE_ESTIMATION_PASS,
                m_width, m_height);
			double time = sutilCurrentTime() - start;
			//m_logger->log("6/%d DIRECT_RADIANCE_ESTIMATION_PASS time: %1.3fs\n", numSteps, time);
        }

        //
        // Combine indirect and direct buffers in the output buffer
        //
		if(generateOutput){
			double start = sutilCurrentTime();
			nvtx::ScopedRange r("OptixEntryPoint::PPM_OUTPUT_PASS");
			m_context->launch(OptixEntryPoint::PPM_OUTPUT_PASS,
				m_width, m_height);
			double time = sutilCurrentTime() - start;
			//m_logger->log("7/%d OUTPUT_PASS time: %1.3fs\n", numSteps, time);
		}

        double traceTime = sutilCurrentTime() -traceStartTime;

		//m_logger->log("---- END Trace time: %1.3fs -----\n", traceTime);
    }
    catch(const optix::Exception & e)
    {
        QString error = QString("An OptiX error occurred: %1").arg(e.getErrorString().c_str());
        throw std::exception(error.toLatin1().constData());
    }
}


void PMOptixRenderer::resizeBuffers(unsigned int width, unsigned int height, unsigned int photonWidth)
{
	m_photonWidth = photonWidth;
	m_context["emittedPhotonsPerIterationFloat"]->setFloat(m_photonWidth * m_photonWidth);
	m_context["photonLaunchWidth"]->setUint(m_photonWidth);
	m_photons->setSize(getNumPhotons());
	m_context["photonsSize"]->setUint(getNumPhotons());
	m_photonsHashCells->setSize( getNumPhotons() );

    m_outputBuffer->setSize( width, height );
    m_raytracePassOutputBuffer->setSize( width, height );
    m_outputBuffer->setSize( width, height );
    m_directRadianceBuffer->setSize( width, height );
    m_indirectRadianceBuffer->setSize( width, height );
    m_randomStatesBuffer->setSize(max(m_photonWidth, (unsigned int)width), max(m_photonWidth,  (unsigned int)height));
    initializeRandomStates();
    m_width = width;
    m_height = height;
}

unsigned int PMOptixRenderer::getWidth() const
{
    return m_width;
}

unsigned int PMOptixRenderer::getHeight() const
{
    return m_height;
}

void PMOptixRenderer::getOutputBuffer( void* data )
{
    void* buffer = reinterpret_cast<void*>( m_outputBuffer->map() );
    memcpy(data, buffer, getScreenBufferSizeBytes());
    m_outputBuffer->unmap();
}

std::vector<unsigned int> PMOptixRenderer::getHitCount()
{
	std::vector<unsigned int> res(m_sceneObjects, 0);

	unsigned int* buffer = reinterpret_cast<unsigned int*>( m_hitCountBuffer->map() );
	for(unsigned int i = 0; i < m_sceneObjects; ++i)
	{
		res[i] = buffer[i];
	}
    m_hitCountBuffer->unmap();

	return res;
}

std::vector<float> PMOptixRenderer::getRadiance()
{
	std::vector<float> res(m_sceneObjects, 0);

	float* buffer = reinterpret_cast<float*>( m_rawRadianceBuffer->map() );
	for(unsigned int i = 0; i < m_sceneObjects; ++i)
	{
		res[i] = buffer[i];
	}
    m_rawRadianceBuffer->unmap();

	return res;
}

float PMOptixRenderer::getEmittedPower()
{
	auto powerEmittedPtr = (float *) m_powerEmittedBuffer->map();
	float res = *powerEmittedPtr / 2.0f;
	m_powerEmittedBuffer->unmap();
	return res;
}

unsigned int PMOptixRenderer::getScreenBufferSizeBytes() const
{
    return m_width*m_height*sizeof(optix::float3);
}

unsigned int PMOptixRenderer::getNumPhotons() const
{
	return m_photonWidth * m_photonWidth * MAX_PHOTON_COUNT;
}

static void transformBufferMatrix(Buffer buffer, const Matrix4x4& matrix)
{
	float3 *bufferHost = (float3*) buffer->map();
	RTsize bufferSize;
	buffer->getSize(bufferSize);

	for(RTsize i = 0; i < bufferSize; ++i)
	{
		float4 homogeneus = matrix * make_float4(bufferHost[i], 1.0f);
		bufferHost[i] = make_float3(homogeneus / homogeneus.w);
	}
	buffer->unmap();
}

void PMOptixRenderer::transformNode(const QString &nodeName, const optix::Matrix4x4 &transformation)
{
	transformNodeImpl(nodeName, transformation, true);
}


void PMOptixRenderer::setNodeTransformation(const QString &nodeName, const optix::Matrix4x4 &transformation)
{
	transformNodeImpl(nodeName, transformation, false);
}

void PMOptixRenderer::setLightDirection(const QString &lightName, const Vector3 &direction)
{
	// update the light buffer if node is a light
	if(!m_lights->contains(lightName)){
		throw std::invalid_argument(("Invalid light: " + lightName).toStdString());
	}

	auto lightIndexes = (*m_lights)[lightName];
	Light* lightsHost = (Light*)m_lightBuffer->map();
	for(auto lightIndexesIt = lightIndexes.cbegin(); lightIndexesIt != lightIndexes.cend(); ++lightIndexesIt){
		lightsHost[*lightIndexesIt].setDirection(direction);
	}
    m_lightBuffer->unmap();
}

void PMOptixRenderer::transformNodeImpl(const QString &nodeName, const optix::Matrix4x4 &transformation, bool preMultiply)
{
	if(nodeName == NULL || nodeName.isEmpty())
	{
		throw std::invalid_argument("nodeName can't be NULL or empty");
	}

	auto group = (*m_groups)[nodeName];
	if(group == NULL)
	{
		throw std::invalid_argument((nodeName + " doesn't exists").toStdString());
	}
	unsigned int childCount = group->getChildCount();
	if(childCount == 0)
	{
		throw std::invalid_argument((nodeName + " has no geometries").toStdString());
	}
	
	// apply transform to every thing in on group
	for(unsigned int childIdx = 0; childIdx < childCount; ++childIdx)
	{
		auto transform = group->getChild<Transform>(childIdx);
		
		// if premultiply is true it takes into account the previous transformation
		// otherwise it simply overwrites it
		if(preMultiply)
		{
			float transformMatrixData[16];
			transform->getMatrix(false, transformMatrixData, NULL);
			Matrix4x4 transformMatrix(transformMatrixData);
			Matrix4x4 resMatrix = transformation * transformMatrix;
			transform->setMatrix(false, resMatrix.getData(), NULL);
		} 
		else 
		{
			transform->setMatrix(false, transformation.getData(), NULL);
		}
	}
	m_context["sceneRootObject"]->getGroup()->getAcceleration()->markDirty();
	group->getAcceleration()->markDirty();


	// update the light buffer if node is a light
	if(m_lights->contains(nodeName))
	{
		auto lightIndexes = (*m_lights)[nodeName];
		Light* lightsHost = (Light*)m_lightBuffer->map();
		for(auto lightIndexesIt = lightIndexes.cbegin(); lightIndexesIt != lightIndexes.cend(); ++lightIndexesIt)
		{
			if(preMultiply)
			{
				lightsHost[*lightIndexesIt].transform(transformation);
			}
			else
			{
				lightsHost[*lightIndexesIt].setTransform(transformation);
			}
		}
        m_lightBuffer->unmap();
	}
}

int PMOptixRenderer::deviceOrdinal() const
{
	return m_optixDeviceOrdinal;
}

Buffer PMOptixRenderer::outputBuffer()
{
	return m_outputBuffer;
}

unsigned int PMOptixRenderer::totalPhotons()
{
	return m_photonWidth * m_photonWidth;
}

const std::vector<std::string>& PMOptixRenderer::objectToNameMapping() const
{
	return m_objectIdToName;
}

Program PMOptixRenderer::createProgram(const std::string& filename, const std::string programName)
{
	try {
		return m_context->createProgramFromPTXFile(relativePathToExe(filename), programName );
	}
	catch(std::exception& ex)
	{
		throw std::invalid_argument("Error creating program " + filename + ": " + ex.what());
	}
}
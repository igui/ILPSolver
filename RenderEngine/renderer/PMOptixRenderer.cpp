/* 
 * Copyright (c) 2013 Opposite Renderer
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
#include <sstream>
#include "renderer/RayType.h"
#include "ComputeDevice.h"
#include "clientserver/RenderServerRenderRequest.h"
#include <exception>
#include "util/sutil.h"
#include "scene/Scene.h"
#include "renderer/helpers/nsight.h"

const unsigned int PMOptixRenderer::PHOTON_GRID_MAX_SIZE = 100*100*100;
const unsigned int PMOptixRenderer::MAX_PHOTON_COUNT = MAX_PHOTONS_DEPOSITS_PER_EMITTED;
using namespace optix;


PMOptixRenderer::PMOptixRenderer() : 
    m_initialized(false),
    m_width(10),
    m_height(10),
	m_photonWidth(10)
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

    m_context["maxPhotonDepositsPerEmitted"]->setUint(MAX_PHOTON_COUNT);
    m_context["ppmRadius"]->setFloat(0.f);
    m_context["ppmRadiusSquared"]->setFloat(0.f);
    m_context["emittedPhotonsPerIterationFloat"]->setFloat(0.f);
    m_context["photonLaunchWidth"]->setUint(0);

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
        Program generatorProgram = m_context->createProgramFromPTXFile( "PMRayGenerator.cu.ptx", "generateRay" );
        Program exceptionProgram = m_context->createProgramFromPTXFile( "PMRayGenerator.cu.ptx", "exception" );
        Program missProgram = m_context->createProgramFromPTXFile( "PMRayGenerator.cu.ptx", "miss" );
        
        m_context->setRayGenerationProgram( OptixEntryPoint::PPM_RAYTRACE_PASS, generatorProgram );
        m_context->setExceptionProgram( OptixEntryPoint::PPM_RAYTRACE_PASS, exceptionProgram );
        m_context->setMissProgram(RayType::RADIANCE, missProgram);
        m_context->setMissProgram(RayType::RADIANCE_IN_PARTICIPATING_MEDIUM, missProgram);
    }

    //
    // Photon Tracing OptixEntryPoint
    //

    {
        Program generatorProgram = m_context->createProgramFromPTXFile( "PMPhotonGenerator.cu.ptx", "generator" );
        Program exceptionProgram = m_context->createProgramFromPTXFile( "PMPhotonGenerator.cu.ptx", "exception" );
        Program missProgram = m_context->createProgramFromPTXFile( "PMPhotonGenerator.cu.ptx", "miss");
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


    //
    // Indirect Radiance Estimation Buffer
    //

    m_indirectRadianceBuffer = m_context->createBuffer( RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT3, m_width, m_height );
    m_context["indirectRadianceBuffer"]->set( m_indirectRadianceBuffer );
    
    //
    // Indirect Radiance Estimation Program
    //
    {
        Program program = m_context->createProgramFromPTXFile( "PMIndirectRadianceEstimation.cu.ptx", "kernel" );
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
        Program program = m_context->createProgramFromPTXFile( "PMDirectRadianceEstimation.cu.ptx", "kernel" );
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
        Program program = m_context->createProgramFromPTXFile( "PMOutput.cu.ptx", "kernel" );
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
        m_sceneRootGroup = scene.getSceneRootGroup(m_context);
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

        // Add the lights from the scene to the light buffer
        m_lightBuffer->setSize(lights.size());
        Light* lights_host = (Light*)m_lightBuffer->map();
        memcpy(lights_host, scene.getSceneLights().constData(), sizeof(Light)*lights.size());
        m_lightBuffer->unmap();

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
	render(512, details.getHeight(), details.getWidth(), details.getCamera(), true);
}

void PMOptixRenderer::genPhotonMap(unsigned int photonLaunchWidth)
{
	render(photonLaunchWidth, 10, 10, Camera(), false);
}

void PMOptixRenderer::render(unsigned int photonLaunchWidth, unsigned int height, unsigned int width, const Camera camera, bool generateOutput)
{
	//m_logger->log("START\n");
    if(!m_initialized)
    {
        throw std::exception("Traced before PMOptixRenderer was initialized.");
    }

	nvtx::ScopedRange r("PMOptixRenderer::Trace");

	//m_logger->log("Used host memory: %1.1fMib\n", m_context->getUsedHostMemory() / (1024.0f * 1024.0f) );


	/*
    Light* lights_host = (Light*)m_lightBuffer->map();
	RTsize n_lights;
	m_lightBuffer->getSize(n_lights);

	for(unsigned int i = 0; i < n_lights; ++i)
	{
		float3 newPosition = (m_sceneAABB.max - m_sceneAABB.min) * ((float) rand() / RAND_MAX) + m_sceneAABB.min;
		lights_host[i].position = newPosition;
	}
    m_lightBuffer->unmap(); */


    try
    {
        // If the width and height of the current render request has changed, we must resize buffers
		if(width != m_width || height != m_height || photonLaunchWidth != m_photonWidth)
        {
			this->resizeBuffers(width, height, photonLaunchWidth);
        }

        double traceStartTime = sutilCurrentTime();
        m_context["camera"]->setUserData( sizeof(Camera), &camera );

		int numSteps = generateOutput ? 7 : 3;

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
        if(generateOutput){
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

unsigned int PMOptixRenderer::getScreenBufferSizeBytes() const
{
    return m_width*m_height*sizeof(optix::float3);
}

unsigned int PMOptixRenderer::getNumPhotons() const
{
	return m_photonWidth * m_photonWidth * MAX_PHOTON_COUNT;
}


std::string PMOptixRenderer::idToObjectName(unsigned int objectId) const
{
	if(objectId < m_objectIdToName.size())
	{
		return m_objectIdToName.at(objectId);
	} else
	{
		return "???";
	}
}
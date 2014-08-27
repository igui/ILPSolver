/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include "render_engine_export_api.h"
#include "OptixRenderer.h"
#include "math/AAB.h"
#include "logging/Logger.h"
#include <vector>

class ComputeDevice;
class RenderServerRenderRequestDetails;
class Scene;
class Camera;

class PMOptixRenderer: public OptixRenderer
{
public:

    RENDER_ENGINE_EXPORT_API PMOptixRenderer();
    RENDER_ENGINE_EXPORT_API ~PMOptixRenderer();

    RENDER_ENGINE_EXPORT_API void initScene(Scene & scene);
    RENDER_ENGINE_EXPORT_API void initialize(const ComputeDevice & device, Logger *logger);

    RENDER_ENGINE_EXPORT_API void renderNextIteration(unsigned long long iterationNumber, unsigned long long localIterationNumber, 
        float PPMRadius, const RenderServerRenderRequestDetails & details);
	RENDER_ENGINE_EXPORT_API void render(unsigned int photonLaunchWidth, unsigned int height, unsigned int width, const Camera camera, bool calculateRadiance);
	RENDER_ENGINE_EXPORT_API void genPhotonMap(unsigned int photonLaunchWidth);
    RENDER_ENGINE_EXPORT_API void getOutputBuffer(void* data);
	RENDER_ENGINE_EXPORT_API std::vector<unsigned int> getHitCount();
    RENDER_ENGINE_EXPORT_API unsigned int getWidth() const;
    RENDER_ENGINE_EXPORT_API unsigned int getHeight() const;
    RENDER_ENGINE_EXPORT_API unsigned int getScreenBufferSizeBytes() const;

    const static float PPM_INITIAL_RADIUS;
    const static unsigned int PHOTON_GRID_MAX_SIZE;

private:
    void initDevice(const ComputeDevice & device);
	void compile();
    void loadObjGeometry( const std::string& filename, optix::Aabb& bbox );
    void initializeRandomStates();
    void createUniformGridPhotonMap(float ppmRadius);
    void initializeStochasticHashPhotonMap(float ppmRadius);
    void createPhotonKdTreeOnCPU();
	unsigned int getNumPhotons() const;

    optix::Buffer m_outputBuffer;
    optix::Buffer m_photons;
    optix::Buffer m_photonKdTree;
    optix::Buffer m_hashmapOffsetTable;
    optix::Buffer m_photonsHashCells;
    optix::Buffer m_raytracePassOutputBuffer;
    optix::Buffer m_directRadianceBuffer;
    optix::Buffer m_indirectRadianceBuffer;
    optix::Group  m_sceneRootGroup;
    optix::Buffer m_volumetricPhotonsBuffer;
    optix::Buffer m_lightBuffer;
    optix::Buffer m_randomStatesBuffer;

    float m_spatialHashMapCellSize;
    AAB m_sceneAABB;
	float m_scenePPMRadius;
    optix::uint3 m_gridSize;
    unsigned int m_spatialHashMapNumCells;

    unsigned int m_width;
    unsigned int m_height;
	unsigned int m_photonWidth;

    bool m_initialized;

    const static unsigned int MAX_BOUNCES;
    const static unsigned int MAX_PHOTON_COUNT;

	void resizeBuffers(unsigned int width, unsigned int height, unsigned int generateOutput);
    optix::Context m_context;
    int m_optixDeviceOrdinal;

	Logger *m_logger;
};
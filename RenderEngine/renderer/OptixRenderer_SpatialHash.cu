/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "config.h"
#include <cuda.h>
#include <curand_kernel.h>
#include <optix_world.h>
#include <thrust/reduce.h>
#include <thrust/pair.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/copy.h>
#include <thrust/partition.h>
#include <thrust/scan.h>
#include <thrust/adjacent_difference.h>
#include "renderer/ppm/Photon.h"
#include <cstdio>
#include <cmath>
#include "renderer/ppm/PhotonGrid.h"
#include "renderer/Hitpoint.h"
#include "renderer/PPMOptixRenderer.h"
#include "renderer/PMOptixRenderer.h"
#include "util/sutil.h"
#include "renderer/OptixEntryPoint.h"
#include "renderer/helpers/optix.h"
#include "renderer/helpers/random.h"
#include "renderer/helpers/nsight.h"
#include "math/Vector3.h"

using namespace optix;

static __host__ __device__ optix::uint3 ceil3f(const Vector3 & f)
{
    optix::uint3 result;
    result.x = (unsigned int)ceil(f.x);
    result.y = (unsigned int)ceil(f.y);
    result.z = (unsigned int)ceil(f.z);
    return result;
}

static optix::uint3 calculateGridSize(const Vector3 & sceneSize, const float & radius )
{
    Vector3 f = sceneSize / radius;
    optix::uint3 result = ceil3f(f);
    result.x = max(1, result.x);
    result.y = max(1, result.y);
    result.z = max(1, result.z);
    return result;
}

template<typename A, typename B>
static __host__ __device__ Vector3 componentWiseDivide(const A & f1, const B & ui2)
{
    Vector3 result;
    result.x = f1.x / ui2.x;
    result.y = f1.y / ui2.y;
    result.z = f1.z / ui2.z;
    return result;
}

static float getSmallestPossibleCellSize(const Vector3 & sceneExtent, const unsigned int maxGridSize)
{
    float sceneVolume = sceneExtent.x*sceneExtent.y*sceneExtent.z;
    float minVolumePerCell = sceneVolume/maxGridSize;
    float smallestPossibleRadiusC = powf(minVolumePerCell, 1.0f/3.0f);
    Vector3 numCellsF = sceneExtent/smallestPossibleRadiusC;
    uint3 numCells = floor3f(numCellsF);
    Vector3 radiusEachAxis = componentWiseDivide(sceneExtent, numCells);
    return fmaxf(radiusEachAxis);
}

#if ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_UNIFORM_GRID

/*
// Construct acceleration structure.
*/

// Reduction type
struct AABB
{
    __host__ __device__ AABB(){}
    __host__ __device__ AABB(float3 _first, float3 _second, bool _valid, int _numPhotons)
    {
        first = _first;
        second = _second;
        valid = _valid;
        numPhotons = _numPhotons;
    }
    float3 first;
    float3 second;
    unsigned int numPhotons;
    unsigned char valid;
};

// reduce a pair of AABB's (a,b) to an AABB containing a and b
struct AABBReducer : public thrust::binary_function<AABB, AABB, AABB>
{
    __host__ __device__ AABB operator()(AABB a, AABB b)
    {
        if(!a.valid) return b;
        if(!b.valid) return a;

        AABB n;
        n.first = make_float3(thrust::min(a.first.x, b.first.x), thrust::min(a.first.y, b.first.y), thrust::min(a.first.z, b.first.z));
        n.second = make_float3(thrust::max(a.second.x, b.second.x), thrust::max(a.second.y, b.second.y), thrust::max(a.second.z, b.second.z));
        n.valid = a.valid * b.valid;
        n.numPhotons = a.valid*a.numPhotons + b.valid*b.numPhotons;
        return n;
    }
};

// convert a photon to a AABB containing that photon
struct PhotonToAABBConverter : public thrust::unary_function<Photon, AABB>
{
    __host__ __device__ AABB operator()(Photon photon)
    {
        AABB a (photon.position, photon.position, fmaxf(photon.power) > 0, fmaxf(photon.power) > 0 ? 1 : 0); 
        return a;
    }
};

static AABB getPhotonsBoundingBox(thrust::device_ptr<Photon> & photons, unsigned int numValidPhotons)
{
    Photon photon = photons[0];
    AABB init = AABB(photon.position, photon.position, fmaxf(photon.power) > 0, fmaxf(photon.power) > 0 ? 1 : 0);
    return thrust::transform_reduce(photons, photons+numValidPhotons, PhotonToAABBConverter(), init, AABBReducer());
}

/*
// Add padding to AABB so that in the final hash table, we will have empty grid cells on the surface of the volume
// (to avoid clamping)
*/

static AABB padAABB(const AABB & in)
{
    AABB result;
    result.first = in.first - 0.0000001f;
    result.second = in.second + 0.0000001f;
    return result;
}

/*
// Calculate hashes
*/

optix::float3 getSceneExtent(const AABB & aabb)
{
    return aabb.second - aabb.first;
}

__global__ void calculateHashCellsKernel(Photon* photons, unsigned int* photonsHashCell, unsigned int* hashCellHistogram,
                                         unsigned int numPhotons, const uint3 gridSize, 
                                         const Vector3 sceneOrigo, const float cellSize, const unsigned int invalidHashCell )
{
    unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
    if(index < numPhotons)
    {
        Photon & photon = photons[index];
        unsigned int hashCell;
        if(fmaxf(photon.power) > 0)
        {
            optix::uint3 hashGridPos = getPhotonGridIndex(photon.position, sceneOrigo, cellSize);
            hashCell = getPhotonGridIndex1D(hashGridPos, gridSize);
            atomicAdd(hashCellHistogram+hashCell, 1);
        }
        else
        {
            hashCell = invalidHashCell;
        }
        photonsHashCell[index] = hashCell;
    }
}

static void calculateHashCells(thrust::device_ptr<Photon> & photons, thrust::device_ptr<unsigned int> & photonsHashCell, thrust::device_ptr<unsigned int> & hashCellHistogram,
                                unsigned int numPhotons, const optix::uint3 & gridSize, 
                                const Vector3 & sceneOrigo, const float radius, const unsigned int invalidHashCell )
{
    const unsigned int blockSize = 512;
    unsigned int numBlocks = numPhotons/blockSize + (numPhotons%blockSize == 0 ? 0 : 1);
    Photon* photonsPtr = thrust::raw_pointer_cast(&photons[0]);
    unsigned int* photonsHashCellPtr = thrust::raw_pointer_cast(&photonsHashCell[0]);
    unsigned int* hashCellHistogramPtr = thrust::raw_pointer_cast(&hashCellHistogram[0]);

    calculateHashCellsKernel<<<numBlocks, blockSize>>> (photonsPtr, photonsHashCellPtr, hashCellHistogramPtr, 
                                                        numPhotons, gridSize, sceneOrigo, radius, invalidHashCell);
}

/*
// Sort photons
*/

static void sortPhotonsByHash(thrust::device_ptr<Photon> & photons, thrust::device_ptr<unsigned int> & photonsHashCell, unsigned int numValidPhotons)
{
    thrust::sort_by_key(photonsHashCell, photonsHashCell+numValidPhotons, photons, thrust::less<unsigned int>());
}

/*
Create photon offset table
*/

static void createHashmapOffsetTable(thrust::device_ptr<unsigned int> & hashmapOffsetTable, unsigned int numHashCells)
{
    // Calculate offsets from histogram by performing an exclusive scan (prefix sum)
    // We add 2 to the length to make sure that the last cell contains the number of photons total
    thrust::exclusive_scan(hashmapOffsetTable, hashmapOffsetTable+numHashCells+1, hashmapOffsetTable, 0);
}

void PPMOptixRenderer::createUniformGridPhotonMap(float ppmRadius)
{
    nvtxRangePushA("Get photon AABB");
    int deviceNumber = 0;
    cudaSetDevice(m_optixDeviceOrdinal);

    // Get a device_ptr to our photon list
    thrust::device_ptr<Photon> photons = getThrustDevicePtr<Photon>(m_photons, deviceNumber);

    // Get the AABB that contains all valid scene photons
    AABB scene = getPhotonsBoundingBox(photons, NUM_PHOTONS);
    AABB extendedScene = padAABB(scene);
    optix::float3 sceneWorldOrigo = extendedScene.first;
    cudaDeviceSynchronize();

    optix::float3 sceneExtent = getSceneExtent(extendedScene);
    nvtxRangePop();

    // Get scene wide maximum radius squared to use for the hash map cell size

    float smallestPossibleCellSize = getSmallestPossibleCellSize(sceneExtent, PHOTON_GRID_MAX_SIZE);
    float cellSize = smallestPossibleCellSize+0.001;

    m_spatialHashMapCellSize = cellSize;

    m_gridSize = calculateGridSize(sceneExtent, cellSize);
    
    // Calculate hashes for photons
    
    unsigned int numHashCells = m_gridSize.x * m_gridSize.y * m_gridSize.z;
    m_spatialHashMapNumCells = numHashCells;

    //printf("# CellSize %.3f, %d hash values, smallestPossibleCellSize: %.3f\n", cellSize, numHashCells, smallestPossibleCellSize);
    //printf("# GridSize %d %d %d\n", gridSize.x, gridSize.y, gridSize.z);

    if(numHashCells > PHOTON_GRID_MAX_SIZE)
    {
        throw std::exception("Too many cells in SpatialHash.cu, over defined PHOTON_GRID_MAX_SIZE.");
    }

    // Calculate hash values for each photon and build the histogram
    unsigned int invalidHashCellValue = numHashCells+1;

    nvtxRangePushA("calculateHashCells and histogram");
    thrust::device_ptr<unsigned int> hashmapOffsetTable = getThrustDevicePtr<unsigned int>(m_hashmapOffsetTable, deviceNumber);
    thrust::fill(hashmapOffsetTable, hashmapOffsetTable+numHashCells, 0);
    thrust::device_ptr<unsigned int> photonsHashCell = getThrustDevicePtr<unsigned int>(m_photonsHashCells, deviceNumber);
    calculateHashCells(photons, photonsHashCell, hashmapOffsetTable, NUM_PHOTONS, m_gridSize, sceneWorldOrigo, cellSize, invalidHashCellValue);
    cudaDeviceSynchronize();
    nvtxRangePop();

    // Sort the photons by their hash value

    nvtxRangePushA("Sort photons by hash");
    sortPhotonsByHash(photons, photonsHashCell, NUM_PHOTONS);
    nvtxRangePop();

    // Calculate the offset table from the histogram
    nvtxRangePushA("Create hashmap offset table");
    createHashmapOffsetTable(hashmapOffsetTable, numHashCells);
    cudaDeviceSynchronize();
    nvtxRangePop();
    
    m_numberOfPhotonsLastFrame = scene.numPhotons;
    //m_numberOfPhotonsInEstimate += m_numberOfPhotonsLastFrame;

    // Update context variables

    m_context["photonsGridCellSize"]->setFloat(cellSize);
    m_context["photonsGridSize"]->setUint(m_gridSize);
    m_context["photonsWorldOrigo"]->setFloat(sceneWorldOrigo);

}

void PMOptixRenderer::createUniformGridPhotonMap(float)
{
    nvtxRangePushA("Get photon AABB");
    int deviceNumber = 0;
    cudaSetDevice(m_optixDeviceOrdinal);

    // Get a device_ptr to our photon list
    thrust::device_ptr<Photon> photons = getThrustDevicePtr<Photon>(m_photons, deviceNumber);

    // Get the AABB that contains all valid scene photons
	AABB scene = getPhotonsBoundingBox(photons, getNumPhotons());
    AABB extendedScene = padAABB(scene);
    optix::float3 sceneWorldOrigo = extendedScene.first;
    cudaDeviceSynchronize();

    optix::float3 sceneExtent = getSceneExtent(extendedScene);
    nvtxRangePop();

    // Get scene wide maximum radius squared to use for the hash map cell size

    float smallestPossibleCellSize = getSmallestPossibleCellSize(sceneExtent, PHOTON_GRID_MAX_SIZE);
    float cellSize = smallestPossibleCellSize+0.001;

    m_spatialHashMapCellSize = cellSize;

    m_gridSize = calculateGridSize(sceneExtent, cellSize);
    
    // Calculate hashes for photons
    
    unsigned int numHashCells = m_gridSize.x * m_gridSize.y * m_gridSize.z;
    m_spatialHashMapNumCells = numHashCells;

    //printf("# CellSize %.3f, %d hash values, smallestPossibleCellSize: %.3f\n", cellSize, numHashCells, smallestPossibleCellSize);
    //printf("# GridSize %d %d %d\n", gridSize.x, gridSize.y, gridSize.z);

    if(numHashCells > PHOTON_GRID_MAX_SIZE)
    {
        throw std::exception("Too many cells in SpatialHash.cu, over defined PHOTON_GRID_MAX_SIZE.");
    }

    // Calculate hash values for each photon and build the histogram
    unsigned int invalidHashCellValue = numHashCells+1;

    nvtxRangePushA("calculateHashCells and histogram");
    thrust::device_ptr<unsigned int> hashmapOffsetTable = getThrustDevicePtr<unsigned int>(m_hashmapOffsetTable, deviceNumber);
    thrust::fill(hashmapOffsetTable, hashmapOffsetTable+numHashCells, 0);
    thrust::device_ptr<unsigned int> photonsHashCell = getThrustDevicePtr<unsigned int>(m_photonsHashCells, deviceNumber);
	calculateHashCells(photons, photonsHashCell, hashmapOffsetTable, getNumPhotons(), m_gridSize, sceneWorldOrigo, cellSize, invalidHashCellValue);
    cudaDeviceSynchronize();
    nvtxRangePop();

    // Sort the photons by their hash value

    nvtxRangePushA("Sort photons by hash");
    sortPhotonsByHash(photons, photonsHashCell, getNumPhotons());
    nvtxRangePop();

    // Calculate the offset table from the histogram
    nvtxRangePushA("Create hashmap offset table");
    createHashmapOffsetTable(hashmapOffsetTable, numHashCells);
    cudaDeviceSynchronize();
    nvtxRangePop();
    
    //m_numberOfPhotonsInEstimate += m_numberOfPhotonsLastFrame;

    // Update context variables

    m_context["photonsGridCellSize"]->setFloat(cellSize);
    m_context["photonsGridSize"]->setUint(m_gridSize);
    m_context["photonsWorldOrigo"]->setFloat(sceneWorldOrigo);

}

#elif ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_STOCHASTIC_HASH

void PPMOptixRenderer::initializeStochasticHashPhotonMap(float ppmRadius)
{
    AAB aabb = m_sceneAABB;
    aabb.addPadding(ppmRadius+0.0001);
    Vector3 sceneExtent = aabb.getExtent();
    float cellSize = ppmRadius;
    m_gridSize = calculateGridSize(sceneExtent, cellSize);
    m_context["photonsGridCellSize"]->setFloat(cellSize);
    m_context["photonsGridSize"]->setUint(m_gridSize);
    m_context["photonsWorldOrigo"]->setFloat(aabb.min);

    // Clear photons
    {
        nvtx::ScopedRange r( "OptixEntryPoint::PPM_CLEAR_PHOTONS_UNIFORM_GRID_PASS" );
        m_context->launch( OptixEntryPoint::PPM_CLEAR_PHOTONS_UNIFORM_GRID_PASS, NUM_PHOTONS);
    }
}

void PMOptixRenderer::initializeStochasticHashPhotonMap(float ppmRadius)
{
    AAB aabb = m_sceneAABB;
    aabb.addPadding(ppmRadius+0.0001);
    Vector3 sceneExtent = aabb.getExtent();
    float cellSize = ppmRadius;
    m_gridSize = calculateGridSize(sceneExtent, cellSize);
    m_context["photonsGridCellSize"]->setFloat(cellSize);
    m_context["photonsGridSize"]->setUint(m_gridSize);
    m_context["photonsWorldOrigo"]->setFloat(aabb.min);

    // Clear photons
    {
        nvtx::ScopedRange r( "OptixEntryPoint::PPM_CLEAR_PHOTONS_UNIFORM_GRID_PASS" );
        m_context->launch( OptixEntryPoint::PPM_CLEAR_PHOTONS_UNIFORM_GRID_PASS, NUM_PHOTONS);
    }
}

#endif

/*
// Initialize random state buffer
*/

static void __global__ initRandomStateBuffer(RandomState* states, unsigned int seed, unsigned int num)
{
    unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
    if(index < num)
    {
        initializeRandomState(&states[index], seed, index);
    }
}

static void initializeRandomStateBuffer(optix::Buffer & buffer, int numStates)
{
    unsigned int seed = 574133*(unsigned int)clock() + 47844152748*(unsigned int)time(NULL);
    RandomState* states = getDevicePtr<RandomState>(buffer, 0);
    const int blockSize = 256;
    int numBlocks = numStates/blockSize + (numStates % blockSize == 0 ? 0 : 1);
    initRandomStateBuffer<<<numBlocks, blockSize>>>(states, seed, numStates);
    cudaDeviceSynchronize();
}

void PPMOptixRenderer::initializeRandomStates()
{
    RTsize size[2];
    m_randomStatesBuffer->getSize(size[0], size[1]);
    int num = size[0]*size[1];
    initializeRandomStateBuffer(m_randomStatesBuffer, num);
}

void PMOptixRenderer::initializeRandomStates()
{
    RTsize size[2];
    m_randomStatesBuffer->getSize(size[0], size[1]);
    int num = size[0]*size[1];
    initializeRandomStateBuffer(m_randomStatesBuffer, num);
}

// From https://devtalk.nvidia.com/default/topic/458062/atomicadd-float-float-atomicmul-float-float-/
__device__ inline void floatAtomicAdd(float* address, float value)
{
	float old = value;
	float new_old;

	do
	{
		new_old = atomicExch(address, 0.0f);
		new_old += old;
	}while ((old = atomicExch(address, new_old))!=0.0f);
};

__global__ void sumPhotonsHitCount(Photon* photons, unsigned int numPhotons, unsigned int *hitCount, float *rawRadiance)
{
	unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
    if(index < numPhotons)
    {
        Photon & photon = photons[index];
        if(fmaxf(photon.power) > 0)
        {
			atomicAdd(hitCount + photon.objectId, 1);
			floatAtomicAdd(rawRadiance + photon.objectId, photon.power.x + photon.power.y + photon.power.z);
        }
    }
}

void PMOptixRenderer::countHitCountPerObject()
{
	nvtxRangePushA("countHitCountPerObject");
    int deviceNumber = 0;
    cudaSetDevice(m_optixDeviceOrdinal);

	thrust::device_ptr<unsigned int> hitCount = getThrustDevicePtr<unsigned int>(m_hitCountBuffer, deviceNumber);
	thrust::fill(hitCount, hitCount + m_sceneObjects, 0);

	thrust::device_ptr<float> rawRadiance = getThrustDevicePtr<float>(m_rawRadianceBuffer, deviceNumber);
	thrust::fill(rawRadiance, rawRadiance + m_sceneObjects, 0);

	unsigned int numPhotons = getNumPhotons();
	const unsigned int blockSize = 512;
    unsigned int numBlocks = numPhotons/blockSize + (numPhotons%blockSize == 0 ? 0 : 1);

	// Get a device_ptr to our photon list
    thrust::device_ptr<Photon> photons = getThrustDevicePtr<Photon>(m_photons, deviceNumber);
    Photon* photonsPtr = thrust::raw_pointer_cast(&photons[0]);
	unsigned int *hitCountPtr = thrust::raw_pointer_cast(&hitCount[0]);
	float *rawRadiancePtr = thrust::raw_pointer_cast(&rawRadiance[0]);

	sumPhotonsHitCount<<<numBlocks, blockSize>>> (photonsPtr, numPhotons, hitCountPtr, rawRadiancePtr);
	cudaDeviceSynchronize();

	m_totalPhotons = 0;
	unsigned int *hitCountHost = (unsigned int *) m_hitCountBuffer->map();
	for(unsigned int i = 0; i < m_sceneObjects; ++i){
		m_totalPhotons += hitCountHost[i];
	}
	m_hitCountBuffer->unmap();

	nvtxRangePop();
}
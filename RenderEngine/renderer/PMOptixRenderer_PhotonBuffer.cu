/*
* Copyright (c) 2015 Opposite Renderer
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

// From https://devtalk.nvidia.com/default/topic/458062/atomicadd-float-float-atomicmul-float-float-/
__device__ inline void floatAtomicAdd(float* address, float value)
{
	float old = value;
	float new_old;

	do
	{
		new_old = atomicExch(address, 0.0f);
		new_old += old;
	} while ((old = atomicExch(address, new_old)) != 0.0f);
};

__global__ void sumPhotonsHitCount(Photon* photons, unsigned int numPhotons, unsigned int *hitCount, float *rawRadiance)
{
	unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
	if (index < numPhotons)
	{
		Photon & photon = photons[index];
		if (fmaxf(photon.power) > 0)
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
	unsigned int numBlocks = numPhotons / blockSize + (numPhotons%blockSize == 0 ? 0 : 1);

	// Get a device_ptr to our photon list
	thrust::device_ptr<Photon> photons = getThrustDevicePtr<Photon>(m_photons, deviceNumber);
	Photon* photonsPtr = thrust::raw_pointer_cast(&photons[0]);
	unsigned int *hitCountPtr = thrust::raw_pointer_cast(&hitCount[0]);
	float *rawRadiancePtr = thrust::raw_pointer_cast(&rawRadiance[0]);

	sumPhotonsHitCount << <numBlocks, blockSize >> > (photonsPtr, numPhotons, hitCountPtr, rawRadiancePtr);
	cudaDeviceSynchronize();

	nvtxRangePop();
}
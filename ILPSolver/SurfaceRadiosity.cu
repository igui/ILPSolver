#include <cuda.h>
#include <optix_world.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/copy.h>
#include "renderer/helpers/optix.h"
#include "renderer/helpers/nsight.h"
#include "SurfaceRadiosity.h"
#include "renderer/PMOptixRenderer.h"
#include <QImage>

__device__ inline static optix::float3 max(optix::float3 a, optix::float3 b)
{
	 return optix::make_float3(
				a.x > b.x ? a.x : b.x,
                a.y > b.y ? a.y : b.y,
                a.z > b.z ? a.z : b.z
			);
}

__device__ inline static optix::float3 min(optix::float3 a, optix::float3 b)
{
	 return optix::make_float3(
				a.x < b.x ? a.x : b.x,
                a.y < b.y ? a.y : b.y,
                a.z < b.z ? a.z : b.z
			);
}

__global__ void transformFloatToRGB(optix::float3 *floatColorBuffer, optix::uchar3 *byteColorBuffer)
{
	unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;

	auto colorFloat = floatColorBuffer[index];
	auto colorFloatCropped = max(
		optix::make_float3(0.0f),
		min(optix::make_float3(1.0f) , colorFloat)
	);

	optix::uchar3 colorByte;
	colorByte.x = floor(colorFloatCropped.x == 1.0f ? 255 : colorFloatCropped.x * 256.0f);
	colorByte.y = floor(colorFloatCropped.y == 1.0f ? 255 : colorFloatCropped.y * 256.0f);
	colorByte.z = floor(colorFloatCropped.z == 1.0f ? 255 : colorFloatCropped.z * 256.0f);
	byteColorBuffer[index] = colorByte;
}

void SurfaceRadiosity::saveImage()
{
	nvtxRangePushA("convertImageToRGB");
    int deviceNumber = 0;
	cudaSetDevice(renderer->deviceOrdinal());

	auto rendererOutputBuffer = renderer->outputBuffer();
	auto floatOutputBuffer = getThrustDevicePtr<optix::float3>(rendererOutputBuffer, deviceNumber);
	auto floatOutputBufferPtr = thrust::raw_pointer_cast(&floatOutputBuffer[0]);

	thrust::device_vector<optix::uchar3> byteOutputBuffer(sampleImageWidth * sampleImageHeight);
	auto byteOutputBufferPtr = thrust::raw_pointer_cast(&byteOutputBuffer[0]);

	const unsigned int blockSize = 512;
    unsigned int numBlocks = (sampleImageWidth * sampleImageHeight)/blockSize + ((sampleImageWidth * sampleImageHeight) % blockSize == 0 ? 0 : 1);

	transformFloatToRGB<<<numBlocks, blockSize>>> (floatOutputBufferPtr, byteOutputBufferPtr);

	cudaDeviceSynchronize();
	nvtxRangePop();
		
	optix::uchar3 *imageBytes = new optix::uchar3[sampleImageWidth * sampleImageHeight];
	thrust::copy(byteOutputBuffer.begin(), byteOutputBuffer.end(), imageBytes);
	auto image = QImage((uchar *) imageBytes, sampleImageWidth, sampleImageHeight, QImage::Format_RGB888);
	image.mirrored(false, true).save("C:\\Users\\Igui\\Desktop\\image.png");
}
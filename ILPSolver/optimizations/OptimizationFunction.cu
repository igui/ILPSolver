#include <cuda.h>
#include <optix_world.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/copy.h>
#include "renderer/helpers/optix.h"
#include "renderer/helpers/nsight.h"
#include "OptimizationFunction.h"
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

__global__ void transformFloatToRGB(optix::float3 *floatColorBuffer, optix::uchar3 *byteColorBuffer, float invGammaCorrection, int width, int height)
{
	unsigned int srcIndex = blockIdx.x*blockDim.x + threadIdx.x;

	auto colorFloat = floatColorBuffer[srcIndex];
	colorFloat.x = powf(colorFloat.x, invGammaCorrection);
	colorFloat.y = powf(colorFloat.y, invGammaCorrection);
	colorFloat.z = powf(colorFloat.z, invGammaCorrection);
	auto colorFloatCropped = max(
		optix::make_float3(0.0f),
		min(optix::make_float3(1.0f) , colorFloat)
	);

	optix::uchar3 colorByte;
	colorByte.x = floor(colorFloatCropped.x == 1.0f ? 255 : colorFloatCropped.x * 256.0f);
	colorByte.y = floor(colorFloatCropped.y == 1.0f ? 255 : colorFloatCropped.y * 256.0f);
	colorByte.z = floor(colorFloatCropped.z == 1.0f ? 255 : colorFloatCropped.z * 256.0f);
	
	auto x = srcIndex / width;
	auto y = srcIndex % width;
	x = height - 1  - x;
	auto dstIndex = x * width + y;

	byteColorBuffer[dstIndex] = colorByte;
}

void OptimizationFunction::saveImage(const QString& fileName)
{
	// convert float3 image data to 24 bit RGB
	nvtxRangePushA("convertImageToRGB");
    int deviceNumber = 0;
	cudaSetDevice(m_renderer->deviceOrdinal());

	auto rendererOutputBuffer = m_renderer->outputBuffer();
	auto floatOutputBuffer = getThrustDevicePtr<optix::float3>(rendererOutputBuffer, deviceNumber);
	auto floatOutputBufferPtr = thrust::raw_pointer_cast(&floatOutputBuffer[0]);

	int imageSize = sampleImageWidth * sampleImageHeight;
	thrust::device_vector<optix::uchar3> byteOutputBuffer(imageSize);
	auto byteOutputBufferPtr = thrust::raw_pointer_cast(&byteOutputBuffer[0]);

	const unsigned int blockSize = 512;
    unsigned int numBlocks = imageSize/blockSize + (imageSize % blockSize == 0 ? 0 : 1);

	transformFloatToRGB<<<numBlocks, blockSize>>> (floatOutputBufferPtr, byteOutputBufferPtr, 1.0f / gammaCorrection, sampleImageWidth, sampleImageHeight);

	cudaDeviceSynchronize();
	nvtxRangePop();
	
	// read image data and write it in an image object
	optix::uchar3 *imageBytes = new optix::uchar3[imageSize];
	thrust::copy(byteOutputBuffer.begin(), byteOutputBuffer.end(), imageBytes);
	auto image = new QImage((uchar *) imageBytes, sampleImageWidth, sampleImageHeight, QImage::Format_RGB888);
	
	// save image to a temporary file
	saveImageAsync(fileName, image);
}
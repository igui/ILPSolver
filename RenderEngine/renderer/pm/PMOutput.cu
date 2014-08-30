/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <optix.h>
#include <optix_cuda.h>
#include <optixu/optixu_math_namespace.h>

using namespace optix;

rtBuffer<float3, 2> outputBuffer;
rtBuffer<float3, 2> indirectRadianceBuffer;
rtBuffer<float3, 2> directRadianceBuffer;
rtDeclareVariable(uint2, launchIndex, rtLaunchIndex, );

RT_PROGRAM void kernel()
{
    float3 finalRadiance = directRadianceBuffer[launchIndex] + indirectRadianceBuffer[launchIndex];
	outputBuffer[launchIndex] = finalRadiance;
}
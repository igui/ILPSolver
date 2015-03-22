/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include <optix_cuda.h>
#include <cuda_runtime.h>
#include "config.h"
#include "renderer/Light.h"
#include "renderer/ShadowPRD.h"
#include "renderer/RayType.h"
#include "renderer/helpers/helpers.h"
#include "renderer/helpers/samplers.h"
#include "renderer/helpers/random.h"
#include "renderer/ppm/Photon.h"
#include "renderer/ppm/PhotonPRD.h"
#include "math/Sphere.h"

using namespace optix;

rtDeclareVariable(rtObject, sceneRootObject, , );
rtBuffer<Photon, 1> photons;
rtBuffer<RandomState, 2> randomStates;
rtDeclareVariable(uint, maxPhotonDepositsPerEmitted, , );
rtDeclareVariable(uint, photonLaunchWidth, , );
rtBuffer<Light, 1> lights;
rtDeclareVariable(uint2, launchIndex, rtLaunchIndex, );
rtDeclareVariable(uint2, launchDim, rtLaunchDim, );
rtDeclareVariable(Sphere, sceneBoundingSphere, , );
rtBuffer<float> powerEmitted;

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


static __device__ void generatePhotonOriginAndDirection(const Light& light, RandomState& state, const Sphere & boundingSphere, 
    float3& origin, float3& direction, float& photonPowerFactor)
{
    origin = light.position;
    float2 sample1 = getRandomUniformFloat2(&state);

    if(light.lightType == Light::AREA)
    {
        float2 sample2 = getRandomUniformFloat2(&state);
        origin += sample1.x*(optix::float3)light.v1 + sample1.y*(optix::float3)light.v2;
        direction = sampleUnitHemisphere(light.normal, sample2);
    }
    else if(light.lightType == Light::POINT)
    {
        // If the point light is well outside the bounding sphere, we make sure to emit 
        // only at the scene (to emulate a directional light)
        float3 sceneCenterToLight = light.position-boundingSphere.center;
        float lightDistance = length(sceneCenterToLight);
        sceneCenterToLight /= lightDistance;
        bool lightWellOutsideSphere = (lightDistance > 1.5*boundingSphere.radius);
        // If light is far away, send photons at the scene and reduce the power based on the solid angle of the scene bounding sphere
        if(lightWellOutsideSphere)
        {
            float3 pointOnDisc = sampleDisc(sample1, boundingSphere.center, boundingSphere.radius, sceneCenterToLight);
            direction = normalize(pointOnDisc-origin);
            // Solid angle of sample disc calculated with http://planetmath.org/calculatingthesolidangleofdisc
            photonPowerFactor = (1  - lightDistance * rsqrtf(boundingSphere.radius*boundingSphere.radius+lightDistance*lightDistance)) / 2.f;
        }
        else
        {
            direction = sampleUnitSphere(sample1);
        }
    }
    else if(light.lightType == Light::SPOT)
    {
        float3 pointOnDisc = sampleDisc(sample1, origin+light.direction, sinf(light.angle/2), light.direction);
        direction = normalize(pointOnDisc-origin);
    }
	else if(light.lightType == Light::DIRECTIONAL)
	{
		direction = light.direction;

		// Good enough (~0.13)
		// origin = sampleUnitSphere(sample1) * sceneBoundingSphere.radius + sceneBoundingSphere.center;
		// photonPowerFactor = 1;
		
		// Too few photons (~0.00)
		// origin = sampleUnitSphere(sample1) * sceneBoundingSphere.radius * 10 + sceneBoundingSphere.center;
		// photonPowerFactor = 10;

		// Good enough (~0.25)
		// origin = sampleDisc(sample1, (float3)sceneBoundingSphere.center - sceneBoundingSphere.radius * direction, sceneBoundingSphere.radius, direction);
		// photonPowerFactor = 1;

		// Test
		origin = sampleDisc(sample1, (float3)sceneBoundingSphere.center - sceneBoundingSphere.radius * direction, sceneBoundingSphere.radius, direction);
		photonPowerFactor = 1;
	}
}

RT_PROGRAM void generator()
{
    PhotonPRD photonPrd;
    photonPrd.pm_index = (launchIndex.y * photonLaunchWidth + launchIndex.x)*maxPhotonDepositsPerEmitted;
    photonPrd.numStoredPhotons = 0;
    photonPrd.depth = 0;
    photonPrd.weight = 1.0f;
    photonPrd.randomState = randomStates[launchIndex];
	photonPrd.inHole = false;

    int lightIndex = 0;
    if(lights.size() > 1)
    {
        float sample = getRandomUniformFloat(&photonPrd.randomState);
        lightIndex = intmin((int)(sample*lights.size()), lights.size()-1);
    }

    Light light = lights[lightIndex];
    float powerScale = lights.size();

    photonPrd.power = light.power*powerScale;

	floatAtomicAdd(&powerEmitted[0], photonPrd.power.x + photonPrd.power.y + photonPrd.power.y);

    float3 rayOrigin, rayDirection;
   
    float photonPowerFactor = 1.f;
    generatePhotonOriginAndDirection(light, photonPrd.randomState, sceneBoundingSphere, rayOrigin, rayDirection, photonPowerFactor);
    photonPrd.power *= photonPowerFactor;


    Ray photon = Ray(rayOrigin, rayDirection, RayType::PHOTON, 0.0001, RT_DEFAULT_MAX );

#if ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_KD_TREE_CPU || ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_UNIFORM_GRID
    // Clear photons owned by this thread
    for(unsigned int i = 0; i < maxPhotonDepositsPerEmitted; ++i)
    {
        photons[photonPrd.pm_index+i].position = make_float3(0.0f);
        photons[photonPrd.pm_index+i].power = make_float3(0.0f);
    }
#endif

    rtTrace( sceneRootObject, photon, photonPrd );

    randomStates[launchIndex] = photonPrd.randomState;


}

rtDeclareVariable(PhotonPRD, photonPrd, rtPayload, );
RT_PROGRAM void miss()
{
    OPTIX_DEBUG_PRINT(photonPrd.depth, "Photon missed geometry.\n");
}

//
// Exception handler program
//

rtDeclareVariable(float3, exceptionErrorColor, , );
RT_PROGRAM void exception()
{
	const unsigned int code = rtGetExceptionCode();
    printf("Exception Photon: %d!\n", code);
	rtPrintExceptionDetails();
    photonPrd.power = make_float3(0,0,0);
}
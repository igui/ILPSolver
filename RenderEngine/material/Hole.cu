/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <optix.h>
#include <optix_cuda.h>
#include <optixu/optixu_math_namespace.h>
#include "config.h"
#include "renderer/Hitpoint.h"
#include "renderer/helpers/random.h"
#include "renderer/helpers/helpers.h"
#include "renderer/RayType.h"
#include "renderer/RadiancePRD.h"
#include "renderer/ppm/PhotonPRD.h"
#include "renderer/ShadowPRD.h"

using namespace optix;

//
// Scene wide variables
//

rtDeclareVariable(rtObject, sceneRootObject, , );

//
// Ray generation program
//

rtDeclareVariable(uint2, launchIndex, rtLaunchIndex, );

//
// Closest hit material
//

rtDeclareVariable(float3, geometricNormal, attribute geometricNormal, ); 
rtDeclareVariable(float3, shadingNormal, attribute shadingNormal, ); 
rtDeclareVariable(RadiancePRD, radiancePrd, rtPayload, );
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(float, tHit, rtIntersectionDistance, );



RT_PROGRAM void closestHitRadiance()
{
	radiancePrd.flags ^= PRD_IN_HOLE;

    if(radiancePrd.depth <= MAX_RADIANCE_TRACE_DEPTH)
    {
		float3 hitPoint = ray.origin + tHit*ray.direction;
		Ray newRay = Ray(hitPoint, ray.direction, ray.ray_type, 0.0001);
        rtTrace( sceneRootObject, newRay, radiancePrd );
    }
}

/*
// Pass the photon along its way through the glass
*/

rtDeclareVariable(PhotonPRD, photonPrd, rtPayload, );

RT_PROGRAM void closestHitPhoton()
{
	photonPrd.inHole = !photonPrd.inHole;

	photonPrd.depth++;
    if (photonPrd.depth <= MAX_PHOTON_TRACE_DEPTH)
    {
		float3 hitPoint = ray.origin + tHit*ray.direction;
		Ray newRay(hitPoint, ray.direction, ray.ray_type, 0.0001);
        rtTrace(sceneRootObject, newRay, photonPrd);
    }
}

rtDeclareVariable(ShadowPRD, shadowPrd, rtPayload, );

RT_PROGRAM void closestHitShadow()
{
	shadowPrd.inHole = !shadowPrd.inHole;

	float3 hitPoint = ray.origin + tHit*ray.direction;
	Ray newRay(hitPoint, ray.direction, ray.ray_type, 0.0001);
    rtTrace(sceneRootObject, newRay, shadowPrd);
}
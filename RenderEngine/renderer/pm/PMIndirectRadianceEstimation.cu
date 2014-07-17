/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

//#define MAX_DEPTH 20

#include <optix.h>
#include <optix_cuda.h>
#include <optixu/optixu_math_namespace.h>
#include "config.h"
#include "renderer/Light.h"
#include "renderer/ppm/Photon.h"
#include "renderer/RayType.h"
#include "renderer/Hitpoint.h"
#include "renderer/ppm/PhotonGrid.h"
#include "renderer/RadiancePRD.h"

using namespace optix;

rtDeclareVariable(uint2, launchIndex, rtLaunchIndex, );

rtBuffer<Photon, 1> photons;
rtBuffer<Hitpoint, 2> raytracePassOutputBuffer;
rtBuffer<float3, 2> indirectRadianceBuffer;

rtDeclareVariable(float, alpha, , );
rtDeclareVariable(float, emittedPhotonsPerIterationFloat, , );
rtDeclareVariable(float, ppmRadius, ,);
rtDeclareVariable(float, ppmRadiusSquared, ,);
rtDeclareVariable(float, ppmRadiusSquaredNew, ,);

rtDeclareVariable(uint3, photonsGridSize, , );
rtDeclareVariable(float3, photonsWorldOrigo, ,);
rtDeclareVariable(float, photonsGridCellSize, ,);
rtBuffer<uint, 1> hashmapOffsetTable;


__device__ __inline float validPhoton(const Photon & photon, const float distance2, const float radius2, const float3 & hitNormal)
{
    return distance2 <= radius2 && dot(-photon.rayDirection, hitNormal) >= 0; 
}

__device__ __inline float3 photonPower(const Photon & photon, const float distance2, const float radius2)
{
    // Use the gaussian filter from Realistic Image Synthesis Using Photon Mapping, Wann Jensen
    const float alpha = 1.818;
    const float beta = 1.953;
    const float expNegativeBeta = 0.141847;
    float weight = alpha*(1 - (1-exp(-beta*distance2/(2*radius2)))/(1-expNegativeBeta));
    return photon.power*weight;
}

RT_PROGRAM void kernel()
{
    Hitpoint rec = raytracePassOutputBuffer[launchIndex];
    
    float3 indirectAccumulatedPower = make_float3( 0.0f, 0.0f, 0.0f );

    int _dPhotonsVisited = 0;
    int _dCellsVisited = 0;

    if(rec.flags & PRD_HIT_NON_SPECULAR)
    {
        float radius2 = ppmRadiusSquared;
        float radius = ppmRadius;
        
        float invCellSize = 1.f/photonsGridCellSize;
        float3 normalizedPosition = rec.position - photonsWorldOrigo;
        unsigned int x_lo = (unsigned int)max(0, (int)((normalizedPosition.x - radius) * invCellSize));
        unsigned int y_lo = (unsigned int)max(0, (int)((normalizedPosition.y - radius) * invCellSize));
        unsigned int z_lo = (unsigned int)max(0, (int)((normalizedPosition.z - radius) * invCellSize));
     
        unsigned int x_hi = (unsigned int)min(photonsGridSize.x-1, (unsigned int)((normalizedPosition.x + radius) * invCellSize));
        unsigned int y_hi = (unsigned int)min(photonsGridSize.y-1, (unsigned int)((normalizedPosition.y + radius) * invCellSize));
        unsigned int z_hi = (unsigned int)min(photonsGridSize.z-1, (unsigned int)((normalizedPosition.z + radius) * invCellSize));    

        if(x_lo <= x_hi)
        {
            for(unsigned int z = z_lo; z <= z_hi; z++)
            {
                for(unsigned int y = y_lo; y <= y_hi; y++)
                {
                    optix::uint3 cell;
                    cell.x = x_lo;
                    cell.y = y;
                    cell.z = z;
                    unsigned int from = getPhotonGridIndex1D(cell, photonsGridSize);
                    unsigned int to = from + (x_hi-x_lo);

                    unsigned int offset = hashmapOffsetTable[from];
                    unsigned int offsetTo = hashmapOffsetTable[to+1];
                    unsigned int numPhotons = offsetTo-offset;

                    _dCellsVisited++;

                    for(unsigned int i = offset; i < offset+numPhotons; i++)
                    {
                        const Photon & photon = photons[i];
                        float3 diff = rec.position - photon.position;
                        float distance2 = dot(diff, diff);
                        if(validPhoton(photon, distance2, radius2, rec.normal))
                        {
                            indirectAccumulatedPower += photonPower(photon, distance2, radius2);
                        }
                        _dPhotonsVisited++;
                    }

                }
            }
        }


    }

    float3 indirectRadiance = indirectAccumulatedPower * rec.attenuation * (1.0f/(M_PIf*ppmRadiusSquared)) *  (1.0f/emittedPhotonsPerIterationFloat);

    indirectRadianceBuffer[launchIndex] = indirectRadiance;
}
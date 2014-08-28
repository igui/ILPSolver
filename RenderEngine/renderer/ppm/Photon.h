/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once
#include "config.h"

struct Photon
{
#ifdef __CUDACC__
    __device__ __inline Photon(const optix::float3 & power, const optix::float3 & position, const optix::float3 & rayDirection, const optix::uint & objectId)
        : power(power), position(position), rayDirection(rayDirection), objectId(objectId)
    {

    }

    __device__ __inline Photon(void)
    {

    }
#endif
    optix::float3 power;
    optix::float3 position;
    optix::float3 rayDirection;
	optix::uint   objectId; 
#if ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_KD_TREE_CPU
    optix::uint   axis;
#endif
#if ENABLE_PARTICIPATING_MEDIA
    optix::uint numDeposits;
#endif
};

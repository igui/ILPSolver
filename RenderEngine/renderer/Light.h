/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once
#include "math/Vector3.h"
#include "render_engine_export_api.h"

#define LIGHT_MAX_NAMELENGTH 20

class Light
{
public:
    enum LightType {AREA, POINT, SPOT};

#ifndef __CUDACC__
    RENDER_ENGINE_EXPORT_API Light(){};
	// Paralelogram surface light
    RENDER_ENGINE_EXPORT_API Light(const char *name, Vector3 power, Vector3 position, Vector3 v1, Vector3 v2);
	// Point Light
    RENDER_ENGINE_EXPORT_API Light(const char *name, Vector3 power, Vector3 position);
	// Spot Light
    RENDER_ENGINE_EXPORT_API Light(const char *name, Vector3 power, Vector3 position, Vector3 direction, float angle);

#endif

	char name[LIGHT_MAX_NAMELENGTH];
    optix::float3 power;
    optix::float3 position;
    optix::float3 v1;
    optix::float3 v2;
    
    float inverseArea;
    union
    {
        float area; // area
        float angle; // spot
    };

    union
    {
        optix::float3 normal; // area
        optix::float3 direction; // spot
    };

    LightType lightType;
};
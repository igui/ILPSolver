/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Light.h"
#include <optixu/optixu_math_namespace.h>
#include "render_engine_export_api.h"
#include <cstring>



Light::Light(const char *name,  Vector3 power, Vector3 position, Vector3 v1, Vector3 v2 ):
	power(power),
    position(position),
    v1(v1),
    v2(v2),
    lightType(LightType::AREA)
{
	strncpy_s(this->name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);

    optix::float3 crossProduct = optix::cross(v1, v2);
    normal = Vector3(optix::normalize(crossProduct));
    area = length(crossProduct);
    inverseArea = 1.0f/area;
}

Light::Light(const char *name, Vector3 power, Vector3 position)
    : power(power),
    position(position),
    lightType(LightType::POINT)
{
	strncpy_s(this->name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);
}

Light::Light(const char *name,  Vector3 power, Vector3 position, Vector3 direction, float angle )
    : power(power), position(position), direction(direction), angle(angle), lightType(LightType::SPOT)
{
	strncpy_s(this->name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);
	this->name[LIGHT_MAX_NAMELENGTH-1] = '\0';
    direction = optix::normalize(direction);
}

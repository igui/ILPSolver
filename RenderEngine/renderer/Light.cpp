/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Light.h"
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>
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

	initAreaLight(v1, v2);
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

void Light::initAreaLight(Vector3 v1, Vector3 v2)
{
	optix::float3 crossProduct = optix::cross(v1, v2);
    normal = Vector3(optix::normalize(crossProduct));
    area = length(crossProduct);
    inverseArea = 1.0f/area;
}

static optix::float3 applyTransform(const optix::Matrix4x4& A, optix::float3 x)
{
	auto x4 = A * optix::make_float4(x, 1.0f);
	return optix::make_float3(x4 / x4.w);
}

static optix::Matrix4x4 getCenteredTransform(optix::Matrix4x4 transform)
{
	auto col = transform.getCol(3);
	col.x = col.y = col.z = 0;
	transform.setCol(3, col);
	return transform;
}

void Light::transform(const optix::Matrix4x4& transform)
{
	position = applyTransform(transform, position);
	optix::Matrix4x4 centeredTransform;

	switch(lightType)
	{
	case AREA: 
		centeredTransform = getCenteredTransform(transform);
		v1 = applyTransform(centeredTransform, v1);
		v2 = applyTransform(centeredTransform, v2);
		initAreaLight(v1, v2);
		break;
	case SPOT:
		direction = optix::normalize(
			applyTransform(getCenteredTransform(transform), direction)
		);
	}
}
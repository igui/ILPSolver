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



Light Light::createParalelogram(const char *name,  Vector3 power, Vector3 position, Vector3 v1, Vector3 v2 )
{
	Light res;
	res.power = Vector3(1.0f);
	res.position = position;
	res.originalPosition = position;
	res.v1 = v1;
	res.originalV1 = v1;
	res.v2 = v2;
	res.originalV2 = v2;
	res.lightType = LightType::AREA;
	res.originalDirection = optix::make_float3(0);
	strncpy_s(res.name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);

	res.initAreaLight(v1, v2);
	return res;
}

Light Light::createPoint(const char *name, Vector3 power, Vector3 position)
{
	Light res;
	res.power = power;
	res.position = position;
	res.originalPosition = position;
	res.lightType = LightType::POINT;
	res.originalV1 = optix::make_float3(0);
	res.originalV2 = optix::make_float3(0);
	res.originalDirection = optix::make_float3(0);
	strncpy_s(res.name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);
	return res;
}

Light Light::createSpot(const char *name,  Vector3 power, Vector3 position, Vector3 direction, float angle )
{
	Light res;
	res.power = power;
	res.position = position;
	res.originalPosition = position;
	res.direction = direction;
	res.originalDirection = direction;
	res.angle = angle;
	res.lightType = LightType::SPOT;
	res.originalV1 = optix::make_float3(0); 
	res.originalV2 = optix::make_float3(0);
	strncpy_s(res.name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);
    res.direction = optix::normalize(direction);
	return res;
}

Light Light::createDirectional(const char *name, Vector3 power, Vector3 direction)
{
	Light res;
	res.power = power;
	res.position = optix::make_float3(0);
	res.originalPosition = optix::make_float3(0);
	res.direction =  direction * (1 / direction.length());
	res.originalDirection = res.direction;
	res.lightType = LightType::DIRECTIONAL;
	res.originalV1 = optix::make_float3(0);
	res.originalV2 = optix::make_float3(0);
	strncpy_s(res.name, LIGHT_MAX_NAMELENGTH, name, _TRUNCATE);
	return res;
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
	transformImpl(transform, true);
}

void Light::setTransform(const optix::Matrix4x4& transform)
{
	transformImpl(transform, false);
}

void Light::transformImpl(const optix::Matrix4x4& transform, bool preMultiply)
{
	position = applyTransform(transform, preMultiply ? position: originalPosition);
	optix::Matrix4x4 centeredTransform;
	switch(lightType)
	{
	case AREA: 
		centeredTransform = getCenteredTransform(transform);
		v1 = applyTransform(centeredTransform, preMultiply ? v1: originalV1);
		v2 = applyTransform(centeredTransform, preMultiply ? v2: originalV2);
		initAreaLight(v1, v2);
		break;
	case SPOT: case DIRECTIONAL:
		direction = optix::normalize(
			applyTransform(
				getCenteredTransform(transform), 
				preMultiply ? direction : originalDirection
			)
		);
	}
}
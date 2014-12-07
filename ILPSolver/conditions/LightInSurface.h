#pragma once

#include "Condition.h"
#include "math/Vector3.h"
#include "scene/Scene.h"
#include "LightInSurfacePosition.h"
#include <optixu_matrix_namespace.h>


class LightInSurface: public Condition
{
public:
	LightInSurface(Scene *scene, const QString& lightId, const QString& surfaceId);

	virtual ConditionPosition *findNeighbour(ConditionPosition *center, float radius, unsigned int retries) const;
	virtual ConditionPosition *initial() const;
	virtual QStringList header() const;
	virtual ~LightInSurface();
private:
	bool pointInSurface(optix::float2 point) const;
	optix::float3 generatePointNeighbourhood(optix::float3 center, float radius, unsigned int& retries) const;
private:
	QString m_lightId;
	QString m_surfaceId;
	optix::float3 base; // quad origin in world coordinates

	// orthonormal base of the plane containing the surface.
	// A point in the plane is represented as: c + alfa * u + beta * v
	//										   where alfa and beta are arbitrary numbers 
	optix::float3 u, v; 

	optix::float2 a,b,c; // other points in uv coordinates (base is 0,0 in uv coordinates).

	float maxDistance; // max distance between two point of the quad
};


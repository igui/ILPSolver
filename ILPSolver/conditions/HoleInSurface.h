#pragma once
#include "Condition.h"
#include "scene/Scene.h"
#include <QString>
#include <optixu_matrix_namespace.h>

class HoleInSurface : public Condition
{
public:
	HoleInSurface(Scene *scene, const QString& nodeId, const QString& surfaceId);
	
	virtual ConditionPosition *findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const;
	virtual ConditionPosition *initial() const;
	virtual QStringList header() const;
private:
	bool pointInSurface(optix::float2 point) const;
	optix::float3 generatePointNeighbourhood(optix::float3 center, float radius, unsigned int& retries) const;
private:
	QString m_nodeId;
	optix::float3 base; // quad origin in world coordinates

	// orthonormal base of the plane containing the surface.
	// A point in the plane is represented as: c + alfa * u + beta * v
	//										   where alfa and beta are arbitrary numbers 
	optix::float3 u, v; 

	optix::float2 a,b,c; // other points in uv coordinates (base is 0,0 in uv coordinates).

	float maxDistance; // max distance between two point of the quad
};


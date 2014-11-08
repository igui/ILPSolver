#pragma once

#include "math/Vector3.h"
#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include <optixu_matrix_namespace.h>
#include <QString>
#include <QStack>

class LightInSurface
{
public:
	LightInSurface(PMOptixRenderer *renderer, Scene *scene, const QString& lightId, const QString& surfaceId);

	QString lightId();
	bool pushMoveToNeighbourhood(float radius, unsigned int retries);
	void popLastMovement();
	virtual ~LightInSurface();
private:
	bool pointInSurface(optix::float2 point) const;
	optix::float3 generatePointNeighbourhood(optix::float3 center, float radius, unsigned int& retries) const;
private:
	QString m_lightId;
	QString m_surfaceId;
	PMOptixRenderer *renderer;
	QStack<optix::Matrix4x4> savedMovements;
	optix::float3 base; // quad origin in world coordinates

	// orthonormal base of the plane containing the surface.
	// A point in the plane is represented as: c + alfa * u + beta * v
	//										   where alfa and beta are arbitrary numbers 
	optix::float3 u, v; 

	optix::float2 a,b,c; // other points in uv coordinates (base is 0,0 in uv coordinates).

	float maxDistance; // quad area
};


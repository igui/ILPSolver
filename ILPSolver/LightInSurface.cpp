#include "LightInSurface.h"


LightInSurface::LightInSurface(Scene *scene, const QString& lightId, const QString& surfaceId):
	m_surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId)),
	scene(scene)
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());

	QVector<Vector3> objectPoints = scene->getObjectPoints(objectId);
	if(objectPoints.length() != 4)
		throw std::invalid_argument((surfaceId + " must have 4 vertices").toStdString());

	base = objectPoints[0];
	
	optix::float3 pointA = (optix::float3) objectPoints[1] - (optix::float3)objectPoints[0];
	optix::float3 pointB = (optix::float3) objectPoints[2] - (optix::float3)objectPoints[0];

	u = normalize(pointA);
	v = normalize(cross(cross(pointA, pointB), pointA));
	
	a = optix::make_float2(dot(u, pointA), dot(v, pointA));
	b = optix::make_float2(dot(u, pointB), dot(v, pointB));

	optix::float3 pointC = (optix::float3) objectPoints[3] - (optix::float3) objectPoints[0];

	c = optix::make_float2( dot(u, pointC), dot(v, pointC));
}

Vector3 LightInSurface::generatePointNeighbourhood(const Vector3 centerWorldCoordinates, float radius) const
{
	optix::float2 center = optix::make_float2(dot(u, centerWorldCoordinates), dot(v, centerWorldCoordinates));

	optix::float2 res;

	do {
		float angle = 2.0f * M_PI * qrand() / RAND_MAX; // random angle
		res = center + radius * optix::make_float2(cosf(angle), sinf(angle)); // res in uv coordinates
	} while(!pointInSurface(res));

	return res.x * u + res.y * v; // res in world coordinates
}

static float sign(optix::float2 p1, optix::float2 p2, optix::float2 p3)
{
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool LightInSurface::pointInSurface(optix::float2 point) const
{
	const static optix::float2 zero = optix::make_float2(0);

	bool b1 = sign(point, zero, a) < 0.0f;
	bool b2 = sign(point, a, c) < 0.0f;
	bool b3 = sign(point, c, zero) < 0.0f;
	
	bool b4 = sign(point, zero, b) < 0.0f;
	bool b5 = sign(point, b, c) < 0.0f;
	bool b6 = b3;

	return (b1 == b2) && (b2 == b3) || (b4 == b5) && (b5 && b6);
}



LightInSurface::~LightInSurface()
{
}

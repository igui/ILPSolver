#include "LightInSurface.h"


LightInSurface::LightInSurface(PMOptixRenderer *renderer, Scene *scene, const QString& lightId, const QString& surfaceId):
	m_surfaceId(surfaceId),
	renderer(renderer)
{
	int objectId = scene->getObjectId(surfaceId);

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

void LightInSurface::pushMoveToNeighbourhood(float radius)
{
	auto center4 = optix::make_float4(base, 1.0f);

	for(auto transformationIt = savedMovements.begin(); transformationIt != savedMovements.end(); ++transformationIt)
	{
		center4 = (*transformationIt) * center4;
	}

	auto center = optix::make_float3(center4 / center4.w);
	auto neighbour = generatePointNeighbourhood(center, radius);
	
	// saves last movement
	auto displacement = neighbour - center;
	auto displacementTransformation = optix::Matrix4x4().identity().translate(displacement);
	savedMovements.push(displacementTransformation);
}

void LightInSurface::popLastMovement()
{
	savedMovements.pop();
}

optix::float3 LightInSurface::generatePointNeighbourhood(optix::float3 centerWorldCoordinates, float radius) const
{
	optix::float2 center = optix::make_float2(dot(u, centerWorldCoordinates - base), dot(v, centerWorldCoordinates - base));
	optix::float2 res;

	do {
		float angle = 2.0f * M_PI * qrand() / RAND_MAX; // random angle
		res = center + radius * optix::make_float2(cosf(angle), sinf(angle)); // res in uv coordinates
	} while(!pointInSurface(res));

	return res.x * u + res.y * v + base; // res in world coordinates
}

static bool sign(optix::float2 p1, optix::float2 p2, optix::float2 p3)
{
  return ((p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)) < 0.0f;
}

bool LightInSurface::pointInSurface(optix::float2 point) const
{
	const static optix::float2 zero = optix::make_float2(0);

	bool b1 = sign(point, zero, a);
	bool b2 = sign(point, a, c);
	bool b3 = sign(point, c, zero);
	
	bool b4 = sign(point, zero, b);
	bool b5 = sign(point, b, c);
	bool b6 = b3;

	return (b1 == b2) && (b2 == b3) || (b4 == b5) && (b5 == b6);
}



LightInSurface::~LightInSurface()
{
}

#include "HoleInSurface.h"
#include "HoleInSurfacePosition.h"
#include <cmath>
#include <qDebug>

HoleInSurface::HoleInSurface(Scene *scene, const QString& nodeId, const QString& surfaceId):
	m_nodeId(nodeId)
{
	int holeObjectId = scene->getObjectId(nodeId);
	if(holeObjectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
	/*QVector<Vector3> holePoints = scene->getObjectPoints(holeObjectId);
	qDebug("%s: %d vertices", qPrintable(nodeId), holePoints.length());
	for(auto point: holePoints){
		qDebug() << "Point: " << point.x << point.y << point.z;
	}*/


	int objectId = scene->getObjectId(surfaceId);

	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());

	QVector<Vector3> objectPoints = scene->getObjectPoints(objectId);
	if(objectPoints.length() < 4)
		throw std::invalid_argument((surfaceId + " must have 4 vertices").toStdString());

	/*qDebug("%s: %d vertices", qPrintable(surfaceId), objectPoints.length());
	for(auto point: objectPoints){
		qDebug() << "Point: " << point.x << point.y << point.z;
	}*/

	base = objectPoints[0];
	
	optix::float3 pointA = (optix::float3) objectPoints[1] - (optix::float3)objectPoints[0];
	optix::float3 pointB = (optix::float3) objectPoints[2] - (optix::float3)objectPoints[0];

	u = normalize(pointA);
	v = normalize(cross(cross(pointA, pointB), pointA));
	
	a = optix::make_float2(dot(u, pointA), dot(v, pointA));
	b = optix::make_float2(dot(u, pointB), dot(v, pointB));

	optix::float3 pointC = (optix::float3) objectPoints[3] - (optix::float3) objectPoints[0];

	c = optix::make_float2( dot(u, pointC), dot(v, pointC));

	maxDistance = 
		std::max(
				std::max(
					std::max(
						length(base - pointA),
						length(base - pointB)
					),
					length(base - pointC)
				),
				std::max(
					std::max(
						length(pointA - pointB),
						length(pointA - pointC)
					),
					length(pointB - pointC)
				)
			);
}

ConditionPosition *HoleInSurface::findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const
{
	auto center4 =  ((HoleInSurfacePosition *) from)->transformation() * optix::make_float4(base, 1.0f);
	auto center = optix::make_float3(center4 / center4.w);
	auto neighbour = generatePointNeighbourhood(center, radius, retries);

	if(retries <= 0){
		return NULL; // no neighbour
	}
	
	// saves last movement
	auto displacement = neighbour - center;
	auto displacementTransformation = optix::Matrix4x4().identity().translate(displacement);
	// applies currentTransformation to the renderer
	auto currentTransformation = displacementTransformation * ((HoleInSurfacePosition *) from)->transformation();
	return new HoleInSurfacePosition(m_nodeId, base, currentTransformation);
}

optix::float3 HoleInSurface::generatePointNeighbourhood(optix::float3 centerWorldCoordinates, float radius, unsigned int& retries) const
{
	auto relativeCenter = centerWorldCoordinates - base;
	auto center = optix::make_float2(dot(u, relativeCenter), dot(v, relativeCenter));
	auto relativeRadius = radius * maxDistance;

	optix::float2 res;
	while(retries > 0) {
		float angle = 2.0f * M_PI * qrand() / RAND_MAX; // random angle
		res = center + relativeRadius * optix::make_float2(cosf(angle), sinf(angle)); // res in uv coordinates
		if(pointInSurface(res)){
			return res.x * u + res.y * v + base; // res in world coordinates
		}
		--retries;
	}
	// <NaN, NaN, NaN> for saying that is no neighbour
	return optix::make_float3(std::numeric_limits<float>::quiet_NaN());
}

static bool sign(optix::float2 p1, optix::float2 p2, optix::float2 p3)
{
  return ((p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)) < 0.0f;
}

bool HoleInSurface::pointInSurface(optix::float2 point) const
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

ConditionPosition *HoleInSurface::initial() const 
{
	return new HoleInSurfacePosition(m_nodeId, base, optix::Matrix4x4::identity());
}

QStringList HoleInSurface::header() const
{
	return QStringList() << (m_nodeId + "(x)") << (m_nodeId + "(y)") << (m_nodeId + "(z)");
}
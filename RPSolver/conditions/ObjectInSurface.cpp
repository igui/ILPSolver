#include "ObjectInSurface.h"
#include "ObjectInSurfacePosition.h"
#include <cmath>
#include <qDebug>


static void checkVertexIndex(const QString& surfaceId, const QVector<Vector3>& objectPoints, int index, const QString &vertexName)
{
	if (index > objectPoints.length() || index < 0){
		throw std::invalid_argument(
			qPrintable(
				QString("%1: invalid vertex %2 index: %3")
					.arg(surfaceId)
					.arg(vertexName)
					.arg(index)
			)
		);
	}
}

static float getMaxDistance(optix::float3 base, optix::float3 pointA, optix::float3 pointB, optix::float3 pointC)
{
	return std::max(
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

static QVector<Vector3> getAndCheckNodeInScene(Scene *scene, const QString& objectName)
{
	int objectId = scene->getObjectId(objectName);
	if (objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + objectName + " in the scene").toStdString());

	auto objectPoints = scene->getObjectPoints(objectId);

	/*qDebug("%s: %d vertices", qPrintable(objectName), objectPoints.length());
	for (int i = 0; i < objectPoints.length(); ++i){
		auto point = objectPoints.at(i);
		qDebug() << "Point " << i << ": " << point.x << point.y << point.z;
	}*/

	return objectPoints;
}

ObjectInSurface::ObjectInSurface(Scene *scene,
	const QString& nodeId,
	const QString& surfaceId,
	const QString& childLightId,
	int surfaceVertexAIndex,
	int surfaceVertexBIndex,
	int surfaceVertexCIndex,
	int surfaceVertexDIndex
	) :
	m_nodeId(nodeId),
	childLightId(childLightId)
{
	getAndCheckNodeInScene(scene, nodeId);
	auto surfacePoints = getAndCheckNodeInScene(scene, surfaceId);
	
	checkVertexIndex(surfaceId, surfacePoints, surfaceVertexAIndex, "A");
	checkVertexIndex(surfaceId, surfacePoints, surfaceVertexBIndex, "B");
	checkVertexIndex(surfaceId, surfacePoints, surfaceVertexCIndex, "C");
	checkVertexIndex(surfaceId, surfacePoints, surfaceVertexDIndex, "D");
	
	base = surfacePoints[surfaceVertexAIndex];

	optix::float3 pointA = (optix::float3) surfacePoints[surfaceVertexBIndex] - (optix::float3)surfacePoints[surfaceVertexAIndex];
	optix::float3 pointB = (optix::float3) surfacePoints[surfaceVertexCIndex] - (optix::float3)surfacePoints[surfaceVertexAIndex];

	u = normalize(pointA);
	v = normalize(cross(cross(pointA, pointB), pointA));

	a = optix::make_float2(dot(u, pointA), dot(v, pointA));
	b = optix::make_float2(dot(u, pointB), dot(v, pointB));

	optix::float3 pointC = (optix::float3) surfacePoints[surfaceVertexDIndex] - (optix::float3) surfacePoints[surfaceVertexAIndex];

	c = optix::make_float2(dot(u, pointC), dot(v, pointC));

	maxDistance = getMaxDistance(base, pointA, pointB, pointC);
}


QVector<float> ObjectInSurface::dimensions() const
{
	return QVector<float>() << length(a) << length(b);
}

ConditionPosition *ObjectInSurface::findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const
{
	auto center4 = ((ObjectInSurfacePosition *)from)->transformation() * optix::make_float4(base, 1.0f);
	auto center = optix::make_float3(center4 / center4.w);
	auto neighbour = generatePointNeighbourhood(center, radius, retries);

	if (retries <= 0){
		return NULL; // no neighbour
	}

	auto normalizedPosition = QVector<float>()
		<< (optix::dot(neighbour - base, u) / length(a))
		<< (optix::dot(neighbour - base, v) / length(b));

	// saves last movement
	auto displacement = neighbour - center;
	auto displacementTransformation = optix::Matrix4x4().identity().translate(displacement);
	// applies currentTransformation to the renderer
	auto currentTransformation = displacementTransformation * ((ObjectInSurfacePosition *)from)->transformation();
	return new ObjectInSurfacePosition(
		m_nodeId,
		childLightId,
		base,
		currentTransformation,
		normalizedPosition
	);
}

static float getRandomAngle()
{
	return 2.0f * M_PI * qrand() / RAND_MAX;
}

optix::float3 ObjectInSurface::generatePointNeighbourhood(optix::float3 centerWorldCoordinates, float radius, unsigned int& retries) const
{
	auto relativeCenter = centerWorldCoordinates - base;
	auto center = optix::make_float2(dot(u, relativeCenter), dot(v, relativeCenter));
	auto relativeRadius = radius * maxDistance;

	while (retries > 0) {
		float angle = getRandomAngle();
		optix::float2 res = center + relativeRadius * optix::make_float2(cosf(angle), sinf(angle)); // res in uv coordinates
		if (pointInSurface(res)){
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

bool ObjectInSurface::pointInSurface(optix::float2 point) const
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

ConditionPosition *ObjectInSurface::initial() const
{
	return new ObjectInSurfacePosition(
		m_nodeId, 
		childLightId,
		base,
		optix::Matrix4x4::identity(), 
		QVector<float>() << 0.0f << 0.0f);
}

QStringList ObjectInSurface::header() const
{
	return QStringList() << (m_nodeId + "(x)") << (m_nodeId + "(y)") << (m_nodeId + "(z)");
}
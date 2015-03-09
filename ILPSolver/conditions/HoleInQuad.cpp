#include "HoleInQuad.h"
#include "HoleInQuadPosition.h"
#include <cmath>

HoleInQuad::HoleInQuad(const QString& nodeId, const optix::float3 base, optix::float3 v1, optix::float3 v2):
	m_nodeId(nodeId)
{
	optix::float3 pointA = v1 - base;
	optix::float3 pointB = v2 - base;

	u = normalize(pointA);
	v = normalize(cross(cross(pointA, pointB), pointA));
	
	a = optix::make_float2(dot(u, pointA), dot(v, pointA));
	b = optix::make_float2(dot(u, pointB), dot(v, pointB));

	optix::float3 pointC = pointA + pointB;

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

ConditionPosition *HoleInQuad::findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const
{
	auto center4 =  ((HoleInQuadPosition *) from)->transformation() * optix::make_float4(base, 1.0f);
	auto center = optix::make_float3(center4 / center4.w);
	auto neighbour = generatePointNeighbourhood(center, radius, retries);

	if(retries <= 0){
		return NULL; // no neighbour
	}
	
	// saves last movement
	auto displacement = neighbour - center;
	auto displacementTransformation = optix::Matrix4x4().identity().translate(displacement);
	// applies currentTransformation to the renderer
	auto currentTransformation = displacementTransformation * ((HoleInQuadPosition *) from)->transformation();
	return new HoleInQuadPosition(m_nodeId, base, currentTransformation);
}

optix::float3 HoleInQuad::generatePointNeighbourhood(optix::float3 centerWorldCoordinates, float radius, unsigned int& retries) const
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

bool HoleInQuad::pointInSurface(optix::float2 point) const
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

ConditionPosition *HoleInQuad::initial() const 
{
	return new HoleInQuadPosition(m_nodeId, base, optix::Matrix4x4::identity());
}

QStringList HoleInQuad::header() const
{
	return QStringList() << (m_nodeId + "(x)") << (m_nodeId + "(y)") << (m_nodeId + "(z)");
}
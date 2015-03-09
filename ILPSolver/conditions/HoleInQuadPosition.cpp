#include "HoleInQuadPosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>


HoleInQuadPosition::HoleInQuadPosition(const QString& nodeName, optix::float3 initialPosition, const optix::Matrix4x4 &transformation):
	nodeName(nodeName),
	initialPosition(initialPosition),
	m_transformation(transformation)
{
}

optix::Matrix4x4 HoleInQuadPosition::transformation() const
{
	return m_transformation;
}

 void HoleInQuadPosition::apply(PMOptixRenderer *renderer) const
 {
	 renderer->setNodeTransformation(nodeName, m_transformation);	
 }


 QStringList HoleInQuadPosition::info() const
{
	QLocale locale; 
	auto position4 = m_transformation * make_float4(initialPosition, 1.0f);
	auto position = optix::make_float3(position4 / position4.w);
	auto x = locale.toString(position.x, 'f', 2);
	auto y = locale.toString(position.y, 'f', 2);
	auto z = locale.toString(position.z, 'f', 2);
	return QStringList() << x << y << z;
}
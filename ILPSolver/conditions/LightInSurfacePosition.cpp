#include "LightInSurfacePosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>

LightInSurfacePosition::LightInSurfacePosition(const QString &lightId, optix::float3 initialPosition, const optix::Matrix4x4 &transformation, const QVector<float>& normalizedPosition):
	lightId(lightId),
	initialPosition(initialPosition),
	m_transformation(transformation),
	m_normalizedPosition(normalizedPosition)
{
}

optix::Matrix4x4 LightInSurfacePosition::transformation() const
{
	return m_transformation;
}

 void LightInSurfacePosition::apply(PMOptixRenderer *renderer) const
 {
	 renderer->setNodeTransformation(lightId, m_transformation);	
 }

QVector<float> LightInSurfacePosition::normalizedPosition() const
{
	return m_normalizedPosition;
}


 QStringList LightInSurfacePosition::info() const
{
	QLocale locale; 
	auto position4 = m_transformation * make_float4(initialPosition, 1.0f);
	auto position = optix::make_float3(position4 / position4.w);
	auto x = locale.toString(position.x, 'f', 2);
	auto y = locale.toString(position.y, 'f', 2);
	auto z = locale.toString(position.z, 'f', 2);
	return QStringList() << x << y << z;
}
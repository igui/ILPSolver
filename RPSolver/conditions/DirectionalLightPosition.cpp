#include "DirectionalLightPosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>
#include <QVector>

DirectionalLightPosition::DirectionalLightPosition(const QString& lightId, const optix::float3& direction):
	m_lightId(lightId),
	m_direction(optix::normalize(direction))
{
}

void DirectionalLightPosition::apply(PMOptixRenderer *renderer) const
{
	renderer->setLightDirection(m_lightId, m_direction);
}

optix::float3 DirectionalLightPosition::direction() const
{
	return m_direction;
}

QString DirectionalLightPosition::lightId() const
{
	return m_lightId;
}

QVector<float> DirectionalLightPosition::normalizedPosition() const
{
	return (QVector<float>() << 
		(1.0f + m_direction.x) / 2.0f << 
		(1.0f + m_direction.y) / 2.0f <<
		(1.0f + m_direction.x) / 2.0f) ;
}

QStringList DirectionalLightPosition::info() const
{
	QLocale locale; 
	auto x = locale.toString(m_direction.x, 'f', 2);
	auto y = locale.toString(m_direction.y, 'f', 2);
	auto z = locale.toString(m_direction.z, 'f', 2);
	return QStringList() << x << y << z;
}
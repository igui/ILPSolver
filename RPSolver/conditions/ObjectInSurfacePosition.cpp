#include "ObjectInSurfacePosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>


ObjectInSurfacePosition::ObjectInSurfacePosition(
	const QString& nodeName,
	const QString& lightName,
	optix::float3 initialPosition,
	const optix::Matrix4x4 &transformation,
	const QVector<float>& normalizedPosition) :
	nodeName(nodeName),
	lightName(lightName),
	initialPosition(initialPosition),
	m_transformation(transformation),
	m_normalizedPosition(normalizedPosition)
{
}

optix::Matrix4x4 ObjectInSurfacePosition::transformation() const
{
	return m_transformation;
}

QVector<float> ObjectInSurfacePosition::normalizedPosition() const
{
	return m_normalizedPosition;
}

void ObjectInSurfacePosition::apply(PMOptixRenderer *renderer) const
{
	renderer->setNodeTransformation(nodeName, m_transformation);
	
	if (!lightName.isEmpty())
	{
		renderer->setNodeTransformation(lightName, m_transformation);
	}
}

optix::float3 ObjectInSurfacePosition::position() const
{
	auto position4 = m_transformation * make_float4(initialPosition, 1.0f);
	return optix::make_float3(position4 / position4.w);
}

QStringList ObjectInSurfacePosition::info() const
{
	QLocale locale;
	auto position = this->position();
	auto x = locale.toString(position.x, 'f', 2);
	auto y = locale.toString(position.y, 'f', 2);
	auto z = locale.toString(position.z, 'f', 2);
	return QStringList() << x << y << z;
}
#include "ObjectInSurfacePosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>


ObjectInSurfacePosition::ObjectInSurfacePosition(const QString& nodeName, optix::float3 initialPosition, const optix::Matrix4x4 &transformation, const QVector<float>& normalizedPosition) :
nodeName(nodeName),
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
}


QStringList ObjectInSurfacePosition::info() const
{
	QLocale locale;
	auto position4 = m_transformation * make_float4(initialPosition, 1.0f);
	auto position = optix::make_float3(position4 / position4.w);
	auto x = locale.toString(position.x, 'f', 2);
	auto y = locale.toString(position.y, 'f', 2);
	auto z = locale.toString(position.z, 'f', 2);
	return QStringList() << x << y << z;
}
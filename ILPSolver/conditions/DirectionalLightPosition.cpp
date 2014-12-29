#include "DirectionalLightPosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>


DirectionalLightPosition::DirectionalLightPosition(const QString& lightId, const optix::float3 direction):
	lightId(lightId),
	direction(direction)
{
}


void DirectionalLightPosition::apply(PMOptixRenderer *renderer) const
{
	renderer->setLightDirection(lightId, direction);
}

QStringList DirectionalLightPosition::info() const
{
	QLocale locale; 
	auto x = locale.toString(direction.x, 'f', 2);
	auto y = locale.toString(direction.y, 'f', 2);
	auto z = locale.toString(direction.z, 'f', 2);
	return QStringList() << x << y << z;
}
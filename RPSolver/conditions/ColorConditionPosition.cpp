#include "ColorConditionPosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>
#include <QVector>
#include <QColor>

ColorConditionPosition::ColorConditionPosition(const QString& node, const optix::float3& hsvColor) :
	m_node(node),
	m_hsvColor(hsvColor)
{
}


/// adapted from http://www.cs.rit.edu/~ncs/color/t_convert.html
optix::float3 ColorConditionPosition::rgbColor() const
{
	QColor color = QColor::fromHsvF(m_hsvColor.x / 360.f, m_hsvColor.y, m_hsvColor.z);
	return optix::make_float3(color.redF(), color.greenF(), color.blueF());
}

void ColorConditionPosition::apply(PMOptixRenderer *renderer) const
{
	renderer->setNodeDiffuseMaterialKd(m_node, rgbColor());
}

optix::float3 ColorConditionPosition::hsvColor() const
{
	return m_hsvColor;
}

float ColorConditionPosition::value() const
{
	return m_hsvColor.z;
}

QString ColorConditionPosition::node() const
{
	return m_node;
}

QVector<float> ColorConditionPosition::normalizedPosition() const
{
	return QVector<float>() << value();
}

QStringList ColorConditionPosition::info() const
{
	QLocale locale;
	auto color = rgbColor();
	auto x = locale.toString(color.x, 'f', 2);
	auto y = locale.toString(color.y, 'f', 2);
	auto z = locale.toString(color.z, 'f', 2);
	return QStringList() << x << y << z;
}
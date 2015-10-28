#include "ColorConditionPosition.h"
#include "renderer/PMOptixRenderer.h"
#include <QLocale>
#include <QVector>

ColorConditionPosition::ColorConditionPosition(const QString& node, const optix::float3& hsvColor) :
	m_node(node),
	m_hsvColor(hsvColor)
{
}


/// adapted from http://www.cs.rit.edu/~ncs/color/t_convert.html
optix::float3 ColorConditionPosition::rgbColor() const
{
	float h = m_hsvColor.x;
	const float& s = m_hsvColor.y;
	const float& v = m_hsvColor.z;

	if (s == 0) {
		// achromatic (grey)
		return optix::make_float3(v);
	}

	h /= 60;			// sector 0 to 5
	int i = floor(h);
	float f = h - i;			// factorial part of h
	float p = v * (1 - s);
	float q = v * (1 - s * f);
	float t = v * (1 - s * (1 - f));

	optix::float3 res;
	float &r = res.x;
	float &g = res.y;
	float &b = res.z;

	switch (i) {
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	default:		// case 5:
		r = v;
		g = p;
		b = q;
		break;
	}

	return res;
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
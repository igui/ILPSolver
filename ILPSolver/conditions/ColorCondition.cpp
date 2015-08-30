#include "ColorCondition.h"
#include "ColorConditionPosition.h"

static float to01Interval(float v)
{
	return std::max(0.f, std::min(v, 1.f));
}

ColorCondition::ColorCondition(const QString& node, const float saturation, const float value):
	node(node)
{
	initialHSV.x = 0;
	initialHSV.y = to01Interval(saturation);
	initialHSV.z = to01Interval(value);
}

ConditionPosition *ColorCondition::findNeighbour(ConditionPosition *from, float radius, unsigned int) const
{
	ColorConditionPosition *position = (ColorConditionPosition *)from;
	auto color = position->hsvColor();

	auto displacement = (2 * qrand() / (float)RAND_MAX - 1.f)  *  radius * 360.0f;
	auto hue = color.x + displacement;
	if (hue < 0)
	{
		hue += 360;
	}
	else if (hue > 360)
	{
		hue -= 360;
	}
	
	color.x = hue;

	return new ColorConditionPosition(position->node(), color);
}

ConditionPosition *ColorCondition::initial() const
{
	return new ColorConditionPosition(node, initialHSV);
}

QVector<float> ColorCondition::dimensions() const
{
	return QVector<float>() << 1.0f;
}

QStringList ColorCondition::header() const
{
	return QStringList() << (node + "(r)") << (node + "(g)") << (node + "(b)");
}


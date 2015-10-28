#include "ColorCondition.h"
#include "ColorConditionPosition.h"

static float to_0_maxInterval(float v, float max)
{
	return std::max(0.f, std::min(v, max));
}

ColorCondition::ColorCondition(const QString& node, const float saturation, const float hue):
	node(node)
{
	initialHSV.x = to_0_maxInterval(hue, 360.0f);
	initialHSV.y = to_0_maxInterval(saturation, 1.0f);
	initialHSV.z = 0;
}

ConditionPosition *ColorCondition::findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const
{
	ColorConditionPosition *position = (ColorConditionPosition *)from;
	auto color = position->hsvColor();

	while (retries > 0){
		auto displacement = (2 * qrand() / (float)RAND_MAX - 1.f)  *  radius;
		auto value = color.z + displacement;

		if (value >= 0 && value <= 1.0)
		{
			color.z = value;
			return new ColorConditionPosition(position->node(), color);
		}

		retries--;
	} 

	return NULL;
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


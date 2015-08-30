#pragma once

#include "Condition.h"
#include <QString>
#include <optixu/optixpp_namespace.h>

class ColorCondition : public Condition
{
public:
	ColorCondition(const QString& node, const float saturation, const float value);
	virtual ConditionPosition *findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const;
	virtual ConditionPosition *initial() const;
	virtual QVector<float> dimensions() const;
	virtual QStringList header() const;
private:
	QString node;
	optix::float3 initialHSV;
};


#pragma once
#include "ConditionPosition.h"
#include <optixu/optixpp_namespace.h>


class ColorConditionPosition : public ConditionPosition
{
public:
	ColorConditionPosition(const QString& node, const optix::float3& hsvColor);
	QString node() const;
	virtual QVector<float> normalizedPosition() const;
	optix::float3 hsvColor() const;
	optix::float3 rgbColor() const;
	virtual void apply(PMOptixRenderer *) const;
	virtual QStringList info() const;
private:
	float hue() const;	

	QString m_node;
	optix::float3 m_hsvColor;
};


#pragma once
#include "ConditionPosition.h"
#include <optixu/optixpp_namespace.h>

class DirectionalLightPosition: public ConditionPosition
{
public:
	DirectionalLightPosition(const QString& lightId, const optix::float3 direction);
	virtual void apply(PMOptixRenderer *) const;
	virtual QStringList info() const;
private:
	QString lightId;
	optix::float3 direction;
};


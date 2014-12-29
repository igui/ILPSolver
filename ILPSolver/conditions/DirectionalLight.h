#pragma once
#include "Condition.h"
#include <QStringList>
#include <optixu/optixpp_namespace.h>

class Scene;

class DirectionalLight : public Condition
{
public:
	DirectionalLight(Scene *scene, const QString& lightId);
	virtual ConditionPosition *findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const;
	virtual ConditionPosition *initial() const;
	virtual QStringList header() const;
private:
	QString m_lightId;
	optix::float3 initialDirection;
};


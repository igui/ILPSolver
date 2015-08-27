#pragma once

#include "ConditionPosition.h"
#include <optixu_matrix_namespace.h>

class PMOptixRenderer;

class ObjectInSurfacePosition : public ConditionPosition
{
public:
	ObjectInSurfacePosition(const QString& nodeName, optix::float3 initialPosition, const optix::Matrix4x4 &transformation, const QVector<float>& normalizedPosition);
	virtual QVector<float> normalizedPosition() const;
	virtual void apply(PMOptixRenderer *) const;
	optix::Matrix4x4 transformation() const;
	virtual QStringList info() const;
	optix::float3 position() const;
private:
	QString nodeName;
	optix::float3 initialPosition;
	optix::Matrix4x4 m_transformation;
	QVector<float> m_normalizedPosition;
};


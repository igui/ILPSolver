#pragma once

#include "ConditionPosition.h"
#include <optixu_matrix_namespace.h>
#include <QStringList>

class PMOptixRenderer;

class LightInSurfacePosition: public ConditionPosition
{
public:
	LightInSurfacePosition(const QString &lightId, optix::float3 initialPosition, const optix::Matrix4x4 &transformation, const QVector<float>& normalizedPosition);
	virtual QVector<float> normalizedPosition() const;
	QStringList info() const;
	optix::Matrix4x4 transformation() const; 
	virtual void apply(PMOptixRenderer *) const;
private:
	QString lightId;
	optix::float3 initialPosition;
	optix::Matrix4x4 m_transformation;
	QVector<float> m_normalizedPosition;
};


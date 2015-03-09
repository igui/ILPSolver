#pragma once

#include "ConditionPosition.h"
#include <optixu_matrix_namespace.h>

class PMOptixRenderer;

class HoleInQuadPosition: public ConditionPosition
{
public:
	HoleInQuadPosition(const QString& nodeName, optix::float3 initialPosition, const optix::Matrix4x4 &transformation);
	virtual void apply(PMOptixRenderer *) const;
	optix::Matrix4x4 transformation() const; 
	virtual QStringList info() const;
private:
	QString nodeName;
	optix::float3 initialPosition;
	optix::Matrix4x4 m_transformation;
};


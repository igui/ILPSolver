#pragma once

#include "ConditionPosition.h"
#include <optixu_matrix_namespace.h>
#include <QString>

class PMOptixRenderer;

class LightInSurfacePosition: public ConditionPosition
{
public:
	LightInSurfacePosition(const QString &lightId, const optix::Matrix4x4 &transformation);
	virtual void apply(PMOptixRenderer *);
private:
	QString lightId;
	optix::Matrix4x4 transformation;
};


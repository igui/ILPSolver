#pragma once

#include <QString>
#include <scene/Scene.h>
#include <vector_types.h>
#include "Interval.h"

class SurfaceRadiosity
{
public:
	SurfaceRadiosity(Scene *scene, const QString &surfaceId);
	Interval evaluate();
	virtual ~SurfaceRadiosity();
private:
	QString surfaceId;
	int objectId;
	Scene *scene;
};


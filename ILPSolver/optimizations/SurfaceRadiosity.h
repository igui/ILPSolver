#pragma once

#include <vector_types.h>
#include <QString>
#include "OptimizationFunction.h"

class Logger;
class PMOptixRenderer;
class Scene;
class Camera;
class QImage;
class QCoreApplication;
class Evaluation;

class SurfaceRadiosity: public OptimizationFunction
{
public:
	SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId);
	virtual QStringList header();
protected:
	virtual Evaluation *genEvaluation();
private:
	QString surfaceId;
	int objectId;
	float surfaceArea;
};


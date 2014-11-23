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
	SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId, float confidenceIntervalRadius);
	virtual Evaluation *evaluate();
	virtual void saveImage(const QString &fileName);
	virtual ~SurfaceRadiosity();
private:
	static const unsigned int sampleImageWidth;
	static const unsigned int sampleImageHeight;
	static const unsigned int defaultPhotonWidth;
	static const float gammaCorrection;

	void saveImageAsync(const QString& fileName, QImage* image);

	QString surfaceId;
	int objectId;
	PMOptixRenderer *renderer;
	float surfaceArea;
	float confidenceIntervalRadius;
	Logger *logger;

	// for sampling images
	Camera *sampleCamera; 
};


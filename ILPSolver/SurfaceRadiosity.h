#pragma once

#include <vector_types.h>
#include <QString>

class Logger;
class PMOptixRenderer;
class Scene;
class Camera;
class QImage;
class QCoreApplication;

class SurfaceRadiosity
{
public:
	SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId);
	bool evaluate();
	QString lastEvaluationInfo();
	void saveImage(const QString &fileName);
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
	float lastRadiosity;
	float maxRadiosity;
	float surfaceArea;
	Logger *logger;

	// for sampling images
	Camera *sampleCamera; 
};


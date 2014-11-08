#pragma once

#include <QString>
#include <vector_types.h>

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
	virtual ~SurfaceRadiosity();
private:
	static const unsigned int sampleImageWidth;
	static const unsigned int sampleImageHeight;
	static const unsigned int defaultPhotonWidth;
	static const float gammaCorrection;

	void saveImage();
	void saveImageAsync(QImage* image);
	QString surfaceId;
	int objectId;
	PMOptixRenderer *renderer;
	float maxRadiosity;
	float surfaceArea;
	Logger *logger;

	// for sampling images
	Camera *sampleCamera; 
};


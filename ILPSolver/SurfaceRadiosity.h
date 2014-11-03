#pragma once

#include <QString>
#include <vector_types.h>

class Logger;
class PMOptixRenderer;
class Scene;
class Camera;

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

	void saveImage();

	QString surfaceId;
	int objectId;
	PMOptixRenderer *renderer;
	Logger *logger;

	// for sampling images
	Camera *sampleCamera; 
	float *sampleOutputBuffer;
};


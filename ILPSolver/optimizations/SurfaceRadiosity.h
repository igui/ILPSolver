#pragma once

#include <vector_types.h>
#include <QStringList>

class Logger;
class PMOptixRenderer;
class Scene;
class Camera;
class QImage;
class QCoreApplication;
class Evaluation;
class SurfaceRadiosityEvaluation;

class SurfaceRadiosity
{
public:
	SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId, float maxRadiosity);
	SurfaceRadiosityEvaluation *evaluateRadiosity();
	SurfaceRadiosityEvaluation *evaluateFast(float quality, bool reusePreviousBuffer);
	void saveImage(const QString &fileName);	
	virtual QStringList header();
	virtual ~SurfaceRadiosity();
private:
	virtual SurfaceRadiosityEvaluation *genEvaluation(int nPhotons);
	void saveImageAsync(const QString& fileName, QImage* image);
private:
	QString surfaceId;
	int objectId;
	float surfaceArea;
	static const unsigned int sampleImageWidth;
	static const unsigned int sampleImageHeight;
	static const unsigned int minPhotonWidth;
	unsigned int maxPhotonWidth;
	static const float gammaCorrection;

	float maxRadiosity;
	PMOptixRenderer *m_renderer;
	Scene *scene;
	Logger *logger;
	// for sampling images
	Camera *sampleCamera; 
};


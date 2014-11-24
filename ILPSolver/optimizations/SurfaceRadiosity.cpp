#include "SurfaceRadiosity.h"
#include "SurfaceRadiosityEvaluation.h"
#include <QImage>
#include <QtCore>
#include <QRunnable>
#include <QTemporaryFile>
#include <optix_world.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>
#include <logging/Logger.h>

const unsigned int SurfaceRadiosity::sampleImageHeight = 768;
const unsigned int SurfaceRadiosity::sampleImageWidth = 1024;
const unsigned int SurfaceRadiosity::defaultPhotonWidth = 512;
const float SurfaceRadiosity::gammaCorrection = 2.8f;

SurfaceRadiosity::SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId, float confidenceIntervalRadius):
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId)),
	sampleCamera(new Camera(scene->getDefaultCamera())),
	renderer(renderer),
	logger(logger),
	confidenceIntervalRadius(confidenceIntervalRadius)
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
	surfaceArea = scene->getObjectArea(objectId);
}

Evaluation *SurfaceRadiosity::evaluateRadiosity()
{
	renderer->render(defaultPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true);
	return genEvaluation();
}

Evaluation *SurfaceRadiosity::evaluateFast()
{
	renderer->genPhotonMap(defaultPhotonWidth);
	return genEvaluation();
}

Evaluation *SurfaceRadiosity::genEvaluation()
{
	const static float z = 1.96f;

	// n is the hit count at the surface
	unsigned int n  = renderer->getHitCount().at(objectId);
	// p is the probability estimate n / (total hits on surfaces)
	float p = (float) n / renderer->totalPhotons();
	// r is the surface radiosity estimate
	float r = renderer->getRadiance().at(objectId) / surfaceArea;
	// radius is the confidence radius given by equations 
	float radius =  z * r * sqrtf(p*(1-p)/n);

	return new SurfaceRadiosityEvaluation(r, radius);
}

class SurfaceRadiosityImageSaveASyncTask : public QRunnable
{
public:
	SurfaceRadiosityImageSaveASyncTask(const QString& fileName, Logger *logger, QImage *image):
		image(image),
		logger(logger),
		fileName(fileName)
	{
	}
private:
	QImage *image;
	Logger *logger;
	QString fileName;

    void run()
    {
		image->save(fileName);
		delete image;
    }
};

void SurfaceRadiosity::saveImageAsync(const QString& fileName, QImage *image)
{
	auto task = new SurfaceRadiosityImageSaveASyncTask(fileName, logger, image);
	QThreadPool::globalInstance()->start(task);
}

QStringList SurfaceRadiosity::header()
{
	return QStringList() << "Radiosity min" << "Radiosity center" << "Radiosity max";
}

SurfaceRadiosity::~SurfaceRadiosity(void)
{
	delete sampleCamera;
}

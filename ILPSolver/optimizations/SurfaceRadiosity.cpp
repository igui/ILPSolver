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
	logger->log("Evaluating solution (With radiosity)\n");
	renderer->render(defaultPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true);
	return genEvaluation();
}

Evaluation *SurfaceRadiosity::evaluateFast()
{
	logger->log("Evaluating solution (Fast)\n");
	renderer->genPhotonMap(defaultPhotonWidth);
	return genEvaluation();
}

Evaluation *SurfaceRadiosity::genEvaluation()
{
	unsigned int n  = renderer->getHitCount().at(objectId);
	float p = (float) n / renderer->totalPhotons();
	float W = renderer->getRadiance().at(objectId) / surfaceArea;

	/*
		// for calculating minimun photonWidth
		static const float 4z2 = 7.6832f;
		float minimumNumPhotons = 4z2 * (1-p) * p * (r/confidenceIntervalRadius)*(r/confidenceIntervalRadius);
		int minimumWidth = ceilf(sqrtf(minimumNumPhotons));
		qDebug() << ("min width: " + QString::number(minimumWidth));
	*/

	const static float z = 1.96f;
	float radius = z * W * sqrtf(p*(1-p)/n);
	return new SurfaceRadiosityEvaluation(W, radius);
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

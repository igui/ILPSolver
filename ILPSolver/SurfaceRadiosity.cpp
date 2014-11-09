#include "SurfaceRadiosity.h"
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

SurfaceRadiosity::SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId):
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId)),
	sampleCamera(new Camera(scene->getDefaultCamera())),
	renderer(renderer),
	logger(logger)
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
	surfaceArea = scene->getObjectArea(objectId);
	maxRadiosity = 0;
}

bool SurfaceRadiosity::evaluate()
{
	logger->log("Evaluating solution\n");
	renderer->render(defaultPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true);
	lastRadiosity = renderer->getRadiance().at(objectId) / surfaceArea;
	maxRadiosity = std::max(maxRadiosity, lastRadiosity);
	logger->log("radiance: %0.1f\n", lastRadiosity);
	return false;
}

QString SurfaceRadiosity::lastEvaluationInfo()
{
	return QString::number(lastRadiosity, 'f', 1);
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

SurfaceRadiosity::~SurfaceRadiosity(void)
{
	delete sampleCamera;
}

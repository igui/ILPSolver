#include "SurfaceRadiosity.h"
#include "SurfaceRadiosityEvaluation.h"
#include <optix_world.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>
#include <QImage>
#include <QtCore>
#include <QRunnable>
#include <QTemporaryFile>
#include <logging/Logger.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>

const unsigned int SurfaceRadiosity::sampleImageHeight = 768;
const unsigned int SurfaceRadiosity::sampleImageWidth = 1024;
const unsigned int SurfaceRadiosity::minPhotonWidth = 16;
const float SurfaceRadiosity::gammaCorrection = 2.8f;

SurfaceRadiosity::SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId):
	m_renderer(renderer),
	scene(scene),
	logger(logger),
	sampleCamera(new Camera(scene->getDefaultCamera())),
	maxPhotonWidth(renderer->getMaxPhotonWidth()),
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId))
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
	surfaceArea = scene->getObjectArea(objectId);
}


SurfaceRadiosityEvaluation *SurfaceRadiosity::genEvaluation(int nPhotons)
{
	const static float z = 1.96f;

	// n is the hit count at the surface
	unsigned int n  = m_renderer->getHitCount().at(objectId);
	// p is the probability estimate n / (total hits on surfaces)
	float p = (float) n / m_renderer->totalPhotons();
	// r is the surface radiosity estimate
	float r = m_renderer->getRadiance().at(objectId) / (surfaceArea);
	float R = m_renderer->getEmittedPower() / (surfaceArea);

	// radius is the confidence radius given by equations 
	float radius = z * R * sqrtf( p*(1-p) /   m_renderer->totalPhotons()  );

	return new SurfaceRadiosityEvaluation(r, radius, nPhotons, nPhotons >= maxPhotonWidth * maxPhotonWidth);
}


QStringList SurfaceRadiosity::header()
{
	return QStringList() << "Radiosity min" << "Radiosity center" << "Radiosity max" << "Photons";
}


SurfaceRadiosity::~SurfaceRadiosity()
{
	delete sampleCamera;
}

SurfaceRadiosityEvaluation *SurfaceRadiosity::evaluateRadiosity()
{
	m_renderer->render(maxPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true, true);
	return genEvaluation(maxPhotonWidth * maxPhotonWidth);
}

SurfaceRadiosityEvaluation *SurfaceRadiosity::evaluateFast(float quality)
{
	int photonWidth = std::max(
				std::min(
					// is good that the photon width is a multiple of 16
					(unsigned int)(maxPhotonWidth * quality) & ~0xF, 
					maxPhotonWidth
				),
				minPhotonWidth
			);

	m_renderer->genPhotonMap(photonWidth);
	return genEvaluation(photonWidth * photonWidth);
}


class ImageSaveASyncTask : public QRunnable
{
public:
	ImageSaveASyncTask(const QString& fileName, Logger *logger, QImage *image):
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
	auto task = new ImageSaveASyncTask(fileName, logger, image);
	QThreadPool::globalInstance()->start(task);
}
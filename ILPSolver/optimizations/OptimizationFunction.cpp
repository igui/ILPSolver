#include "OptimizationFunction.h"
#include <QImage>
#include <QtCore>
#include <QRunnable>
#include <QTemporaryFile>
#include <logging/Logger.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>

const unsigned int OptimizationFunction::sampleImageHeight = 768;
const unsigned int OptimizationFunction::sampleImageWidth = 1024;
const unsigned int OptimizationFunction::minPhotonWidth = 16;
const float OptimizationFunction::gammaCorrection = 2.8f;

OptimizationFunction::OptimizationFunction(PMOptixRenderer *renderer, Scene *scene, Logger *logger):
	m_renderer(renderer),
	scene(scene),
	logger(logger),
	sampleCamera(new Camera(scene->getDefaultCamera())),
	maxPhotonWidth(renderer->getMaxPhotonWidth())
{
}

OptimizationFunction::~OptimizationFunction(void)
{
	delete sampleCamera;
}

Evaluation *OptimizationFunction::evaluateRadiosity()
{
	m_renderer->render(maxPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true, true);
	return genEvaluation(maxPhotonWidth * maxPhotonWidth);
}

Evaluation *OptimizationFunction::evaluateFast(float quality)
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

PMOptixRenderer *OptimizationFunction::renderer()
{
	return m_renderer;
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

void OptimizationFunction::saveImageAsync(const QString& fileName, QImage *image)
{
	auto task = new ImageSaveASyncTask(fileName, logger, image);
	QThreadPool::globalInstance()->start(task);
}
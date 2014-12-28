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
const unsigned int OptimizationFunction::defaultPhotonWidth = 512;
const float OptimizationFunction::gammaCorrection = 2.8f;

OptimizationFunction::OptimizationFunction(PMOptixRenderer *renderer, Scene *scene, Logger *logger):
	m_renderer(renderer),
	scene(scene),
	logger(logger),
	sampleCamera(new Camera(scene->getDefaultCamera()))
{
}

OptimizationFunction::~OptimizationFunction(void)
{
	delete sampleCamera;
}

Evaluation *OptimizationFunction::evaluateRadiosity()
{
	m_renderer->render(defaultPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true);
	return genEvaluation();
}

Evaluation *OptimizationFunction::evaluateFast()
{
	m_renderer->genPhotonMap(defaultPhotonWidth);
	return genEvaluation();
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
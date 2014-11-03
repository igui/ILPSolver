#include "SurfaceRadiosity.h"
#include <optix_world.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>
#include <logging/Logger.h>

const unsigned int SurfaceRadiosity::sampleImageHeight = 768;
const unsigned int SurfaceRadiosity::sampleImageWidth = 1024;
const unsigned int SurfaceRadiosity::defaultPhotonWidth = 512;

SurfaceRadiosity::SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId):
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId)),
	sampleCamera(new Camera(scene->getDefaultCamera())),
	renderer(renderer),
	logger(logger),
	// RGB float output buffer
	sampleOutputBuffer(new float[sampleImageHeight * sampleImageWidth * 3])
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
}

bool SurfaceRadiosity::evaluate()
{
	logger->log("Evaluating solution\n");
	renderer->render(defaultPhotonWidth, sampleImageHeight, sampleImageWidth, *sampleCamera, true);
	saveImage();
	//renderer->genPhotonMap(defaultPhotonWidth);
	return false;
}


SurfaceRadiosity::~SurfaceRadiosity(void)
{
	delete sampleCamera;
	delete[] sampleOutputBuffer;
}

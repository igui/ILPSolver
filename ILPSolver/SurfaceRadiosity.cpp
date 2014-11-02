#include "SurfaceRadiosity.h"
#include <optix_world.h>

SurfaceRadiosity::SurfaceRadiosity(Scene *scene, const QString &surfaceId):
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId)),
	scene(scene)
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
}

bool SurfaceRadiosity::evaluate()
{
	return false;
}

SurfaceRadiosity::~SurfaceRadiosity(void)
{
}

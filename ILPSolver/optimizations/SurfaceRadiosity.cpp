#include "SurfaceRadiosity.h"
#include "SurfaceRadiosityEvaluation.h"
#include <optix_world.h>
#include <scene/Scene.h>
#include <renderer/PMOptixRenderer.h>


SurfaceRadiosity::SurfaceRadiosity(Logger *logger, PMOptixRenderer *renderer, Scene *scene, const QString &surfaceId):
	OptimizationFunction(renderer, scene, logger),
	surfaceId(surfaceId),
	objectId(scene->getObjectId(surfaceId))
{
	if(objectId < 0)
		throw std::invalid_argument(("There isn't any object named " + surfaceId + " in the scene").toStdString());
	surfaceArea = scene->getObjectArea(objectId);
}


Evaluation *SurfaceRadiosity::genEvaluation(int nPhotons)
{
	const static float z = 1.96f;

	// n is the hit count at the surface
	unsigned int n  = renderer()->getHitCount().at(objectId);
	// p is the probability estimate n / (total hits on surfaces)
	float p = (float) n / renderer()->totalPhotons();
	// r is the surface radiosity estimate
	float r = renderer()->getRadiance().at(objectId) / (surfaceArea * nPhotons);
	float R = renderer()->getEmittedPower() / (surfaceArea * nPhotons);

	// radius is the confidence radius given by equations 
	float radius = z * R * sqrtf( p*(1-p) /   renderer()->totalPhotons()  );

	return new SurfaceRadiosityEvaluation(r, radius, nPhotons);
}


QStringList SurfaceRadiosity::header()
{
	return QStringList() << "Radiosity min" << "Radiosity center" << "Radiosity max" << "Photons";
}

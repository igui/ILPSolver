#include "DirectionalLight.h"
#include "DirectionalLightPosition.h"
#include "scene/Scene.h"

DirectionalLight::DirectionalLight(Scene *scene, const QString& lightId):
	m_lightId(lightId),
	initialDirection(optix::make_float3(0))
{
	const Light *light = NULL;
	
	auto sceneLights = scene->getSceneLights();
	for(auto it = sceneLights.cbegin(); it != sceneLights.cend(); ++it){
		if(it->name == lightId){
			light = it;
			break;
		}
	}
	if(!light){
		throw std::invalid_argument(("Light name " + lightId + " is invalid").toStdString());
	}
	if(light->lightType != Light::DIRECTIONAL){
		throw std::invalid_argument(("Light " + lightId + " is not a directional light").toStdString());
	}

	initialDirection = light->direction;
}


ConditionPosition *DirectionalLight::findNeighbour(ConditionPosition *from, float radius, unsigned int) const
{
	// TODO implement
	throw std::logic_error("Not implemented");
}

ConditionPosition *DirectionalLight::initial() const
{
	return new DirectionalLightPosition(m_lightId, initialDirection);
}

QStringList DirectionalLight::header() const
{
	return QStringList() << (m_lightId + "(x)") << (m_lightId + "(y)") << (m_lightId + "(z)");
}
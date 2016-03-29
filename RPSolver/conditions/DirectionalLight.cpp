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

QVector<float> DirectionalLight::dimensions() const
{
	return QVector<float>() << 1.0f << 1.0f << 1.0f;
}

static optix::float2 toSphericalCoord(const optix::float3& v)
{
	return optix::make_float2(
		acosf(v.z),
		atan2f(v.y, v.x)
	);
}

static optix::float3 toCartesianCoord(const optix::float2& phitheta)
{
	auto length = optix::length(phitheta);

    return length * optix::make_float3(
		sinf(phitheta.x) * cosf(phitheta.y),
		sinf(phitheta.x) * sinf(phitheta.y),
		cosf(phitheta.x)
	);
}

ConditionPosition *DirectionalLight::findNeighbour(ConditionPosition *from, float radius, unsigned int) const
{
	auto directionalLightPosition = (DirectionalLightPosition *)from;
	auto sphericalDirection = toSphericalCoord(directionalLightPosition->direction());

	auto sample = qrand() / (float) RAND_MAX;
	auto rotation = 2.0f * M_PI * radius * (optix::make_float2(sample, 1.0f - sample) - 0.5f);
	
	return new DirectionalLightPosition(
		directionalLightPosition->lightId(),
		toCartesianCoord(sphericalDirection + rotation)
	);
}

ConditionPosition *DirectionalLight::initial() const
{
	return new DirectionalLightPosition(m_lightId, initialDirection);
}

QStringList DirectionalLight::header() const
{
	return QStringList() << (m_lightId + "(x)") << (m_lightId + "(y)") << (m_lightId + "(z)");
}
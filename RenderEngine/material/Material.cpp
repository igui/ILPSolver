/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Material.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Material::m_hasLoadedShadowProgramHoleCheck = false;
bool Material::m_hasLoadedShadowProgramFast = false;
optix::Program Material::m_shadowProgramHoleCheck;
optix::Program Material::m_shadowProgramFast;

Material::Material():
	m_objectId(-1)
{
}

Material::~Material()
{

}

void  Material::registerInstanceValues(optix::GeometryInstance & instance)
{
	instance["objectId"]->setUint(this->m_objectId);
	this->registerGeometryInstanceValues(instance);
}

void Material::setObjectId(unsigned int objectId)
{
	this->m_objectId = objectId;
}

void Material::registerMaterialWithShadowProgram( optix::Context & context, optix::Material & material, bool useHoleCheckProgram)
{
	if(useHoleCheckProgram)
	{
		if(!m_hasLoadedShadowProgramHoleCheck)
		{
			m_shadowProgramHoleCheck = context->createProgramFromPTXFile( relativePathToExe("DirectRadianceEstimation.cu.ptx"), "gatherClosestHitOnNonEmmiterHoleCheck");
			m_hasLoadedShadowProgramHoleCheck = true;
		}
		material->setClosestHitProgram(RayType::SHADOW, m_shadowProgramHoleCheck);
	}
	else
	{
		if(!m_hasLoadedShadowProgramFast)
		{
			m_shadowProgramFast = context->createProgramFromPTXFile( relativePathToExe("DirectRadianceEstimation.cu.ptx"), "gatherAnyHitOnNonEmmiterFast");
			m_hasLoadedShadowProgramFast = true;
		}
		material->setAnyHitProgram(RayType::SHADOW, m_shadowProgramFast);
	}
}
/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Material.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Material::m_hasLoadedOptixClosestHitProgram = false;
optix::Program Material::m_optixClosestHitProgram;

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

void Material::registerMaterialWithShadowProgram( optix::Context & context, optix::Material & material )
{
    if(!m_hasLoadedOptixClosestHitProgram)
    {
        m_optixClosestHitProgram = context->createProgramFromPTXFile( relativePathToExe("DirectRadianceEstimation.cu.ptx"), "gatherClosestHitOnNonEmmiter");
        m_hasLoadedOptixClosestHitProgram = true;
    }
	material->setClosestHitProgram(RayType::SHADOW, m_optixClosestHitProgram);
}
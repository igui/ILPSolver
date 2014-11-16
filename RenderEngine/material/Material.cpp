/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Material.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Material::m_hasLoadedOptixAnyHitProgram = false;
optix::Program Material::m_optixAnyHitProgram;

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
    if(!m_hasLoadedOptixAnyHitProgram)
    {
        m_optixAnyHitProgram = context->createProgramFromPTXFile( relativePathToExe("DirectRadianceEstimation.cu.ptx"), "gatherAnyHitOnNonEmitter");
        m_hasLoadedOptixAnyHitProgram = true;
    }
    material->setAnyHitProgram(RayType::SHADOW, m_optixAnyHitProgram);
}
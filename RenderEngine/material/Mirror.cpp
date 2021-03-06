/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Mirror.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Mirror::m_optixMaterialIsCreated = false;
optix::Material Mirror::m_optixMaterial;

Mirror::Mirror(const Vector3 & Kr)
{
    this->Kr = Kr;
}

optix::Material Mirror::getOptixMaterial(optix::Context & context, bool useHoleCheckProgram)
{
    if(!m_optixMaterialIsCreated)
    {
        optix::Program photonProgram = context->createProgramFromPTXFile( relativePathToExe("Mirror.cu.ptx"), "closestHitPhoton");
        optix::Program radianceProgram = context->createProgramFromPTXFile( relativePathToExe("Mirror.cu.ptx"), "closestHitRadiance");
        m_optixMaterial = context->createMaterial();
        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE, radianceProgram);
        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE_IN_PARTICIPATING_MEDIUM, radianceProgram);
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON, photonProgram);
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON_IN_PARTICIPATING_MEDIUM, photonProgram);
		this->registerMaterialWithShadowProgram(context, m_optixMaterial, useHoleCheckProgram);
        m_optixMaterialIsCreated = true;
    }
    return m_optixMaterial;
}

void Mirror::registerGeometryInstanceValues(optix::GeometryInstance & instance )
{
    instance["Kr"]->setFloat(this->Kr);
}

Material* Mirror::clone()
{
	return new Mirror(*this);
}

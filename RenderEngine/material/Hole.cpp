/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Hole.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Hole::m_optixMaterialIsCreated = false;
optix::Material Hole::m_optixMaterial;

Hole::Hole( )
{
}

optix::Material Hole::getOptixMaterial(optix::Context & context, bool useHoleCheckProgram)
{
    if(!m_optixMaterialIsCreated)
    {
        m_optixMaterial = context->createMaterial();
        optix::Program radianceClosestProgram = context->createProgramFromPTXFile( relativePathToExe("Hole.cu.ptx"), "closestHitRadiance");
        optix::Program photonClosestProgram = context->createProgramFromPTXFile( relativePathToExe("Hole.cu.ptx"), "closestHitPhoton");
		optix::Program shadowClosestProgram = context->createProgramFromPTXFile( relativePathToExe("Hole.cu.ptx"), "closestHitPhoton");

        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE, radianceClosestProgram);
        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE_IN_PARTICIPATING_MEDIUM, radianceClosestProgram);
        
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON, photonClosestProgram);
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON_IN_PARTICIPATING_MEDIUM, photonClosestProgram);

		m_optixMaterial->setClosestHitProgram(RayType::SHADOW, shadowClosestProgram);

        m_optixMaterialIsCreated = true;
    }
    
    return m_optixMaterial;
}

/*
// Register any material-dependent values to be available in the optix program.
*/
void Hole::registerGeometryInstanceValues(optix::GeometryInstance & instance )
{
}


Material* Hole::clone()
{
	return new Hole(*this);
}

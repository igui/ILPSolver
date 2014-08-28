/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#pragma once
#include <optixu/optixpp_namespace.h>
class Material
{
public:
	Material();
    virtual ~Material();
    virtual optix::Material getOptixMaterial(optix::Context & context) = 0;
	virtual Material* clone() = 0;
	void registerInstanceValues(optix::GeometryInstance & instance);

	void setObjectId(unsigned int objectId);
protected:
	virtual void registerGeometryInstanceValues( optix::GeometryInstance & instance ) = 0;
    static void registerMaterialWithShadowProgram(optix::Context & context, optix::Material & material);
private:
    static bool m_hasLoadedOptixAnyHitProgram;
    static optix::Program m_optixAnyHitProgram;
	unsigned int m_objectId;
};
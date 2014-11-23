#include "LightInSurfacePosition.h"
#include "renderer/PMOptixRenderer.h"

LightInSurfacePosition::LightInSurfacePosition(const QString &lightId, const optix::Matrix4x4 &transformation):
	lightId(lightId),
	transformation(transformation)
{
}

 void LightInSurfacePosition::apply(PMOptixRenderer *renderer)
 {
	 renderer->setNodeTransformation(lightId, transformation);	
 }

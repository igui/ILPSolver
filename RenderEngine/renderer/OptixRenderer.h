/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include "render_engine_export_api.h"
#include "logging/Logger.h"

class ComputeDevice;
class RenderServerRenderRequestDetails;
class Scene;

class OptixRenderer
{
protected:
	RENDER_ENGINE_EXPORT_API OptixRenderer();
public:
    RENDER_ENGINE_EXPORT_API virtual ~OptixRenderer();

    RENDER_ENGINE_EXPORT_API virtual void initScene(Scene & scene) = 0;
    RENDER_ENGINE_EXPORT_API virtual void initialize(const ComputeDevice & device, Logger *logger) = 0;

    RENDER_ENGINE_EXPORT_API virtual void renderNextIteration(unsigned long long iterationNumber, unsigned long long localIterationNumber, 
        float PPMRadius, const RenderServerRenderRequestDetails & details) = 0;
    RENDER_ENGINE_EXPORT_API virtual void getOutputBuffer(void* data) = 0;
    RENDER_ENGINE_EXPORT_API virtual unsigned int getWidth() const = 0;
    RENDER_ENGINE_EXPORT_API virtual unsigned int getHeight() const = 0;
    RENDER_ENGINE_EXPORT_API virtual unsigned int getScreenBufferSizeBytes() const = 0;
};
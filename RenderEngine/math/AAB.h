/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once
#include "Vector3.h"
#include "math/Sphere.h"
#include "render_engine_export_api.h"

class AAB
{
public:
    Vector3 min;
    Vector3 max;
    RENDER_ENGINE_EXPORT_API AAB(const Vector3 &, const Vector3 &);
    RENDER_ENGINE_EXPORT_API AAB();
    RENDER_ENGINE_EXPORT_API Vector3 getExtent() const;
    RENDER_ENGINE_EXPORT_API Vector3 getCenter() const;
    RENDER_ENGINE_EXPORT_API Sphere getBoundingSphere() const;
    void addPadding(float a)
    {
        min -= a;
        max += a;
    }
};
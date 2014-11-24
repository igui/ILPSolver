/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include "render_engine_export_api.h"
#include <QString>

class Logger
{
public:
	virtual void log(const char *format, ...) = 0;

	RENDER_ENGINE_EXPORT_API virtual ~Logger();
};

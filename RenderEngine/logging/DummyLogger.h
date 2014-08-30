/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include "Logger.h"

class DummyLogger: public Logger
{
public:
	RENDER_ENGINE_EXPORT_API virtual void log(const char *format, ...);
	RENDER_ENGINE_EXPORT_API virtual void log(const QString &styleOptions, const char *format, ...);
};
#pragma once

#include "render_engine_export_api.h"
#include <QString>

class Logger
{
public:
	virtual void log(const char *format, ...) = 0;
	virtual void log(const QString &styleOptions, const char *format, ...) = 0;

	RENDER_ENGINE_EXPORT_API virtual ~Logger();
};

#pragma once

#include "Logger.h"

class DummyLogger: public Logger
{
public:
	RENDER_ENGINE_EXPORT_API virtual void log(const char *format, ...);
	RENDER_ENGINE_EXPORT_API virtual void log(const QString &styleOptions, const char *format, ...);
};
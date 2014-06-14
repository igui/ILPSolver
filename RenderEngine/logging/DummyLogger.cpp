#include "DummyLogger.h"
#include <cstdarg>

void DummyLogger::log(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	vprintf(format, args);
	va_end (args);
}

void DummyLogger::log(const QString &styleOptions, const char *format, ...)
{
	va_list args;
	va_start (args, format);
	vprintf(format, args);
	va_end (args);
}


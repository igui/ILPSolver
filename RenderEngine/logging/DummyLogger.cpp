/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

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


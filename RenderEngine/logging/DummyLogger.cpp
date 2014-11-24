/* 
 * Copyright (c) 2014 Opposite Renderer
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

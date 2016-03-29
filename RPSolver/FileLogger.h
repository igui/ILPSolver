#pragma once

#include "logging/Logger.h"
#include <cstdio>

class FileLogger: public Logger
{
public:
	FileLogger();
	FileLogger(const QString &logPath);
	virtual void log(const char *format, ...);
	virtual ~FileLogger();
private:
	FILE *logStream;
};


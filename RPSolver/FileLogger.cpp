#include "FileLogger.h"

#include <QString>
#include <cerrno>

FileLogger::FileLogger():
	logStream(NULL)
{
}

FileLogger::FileLogger(const QString& path):
	logStream(NULL)
{
	logStream = fopen(path.toStdString().c_str(), "w");
	if(!logStream){
		QString msg = "Couldn't open " + path + ": " + strerror(errno);
		throw std::logic_error(msg.toStdString().c_str());
	}
}

void FileLogger::log(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	vfprintf(logStream, format, args);
	va_end (args);

	va_list args2;
	va_start (args2, format);
	vprintf(format, args2);
	va_end (args2);
}

FileLogger::~FileLogger(void)
{
}

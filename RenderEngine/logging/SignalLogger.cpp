/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "SignalLogger.hxx"

SignalLogger::SignalLogger(QObject *parent):
	QObject(parent),
	signalReceiver(NULL)
{
}

void SignalLogger::log(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	vlog(QString(""), format, args);
	va_end (args);
}


void SignalLogger::vlog(const QString &styleOptions, const char *format, va_list args)
{
	static const size_t bufferSize = 1024;
	char buffer[bufferSize];
	vsprintf_s(buffer, bufferSize, format, args);

	emit newLogMessage(styleOptions, buffer);
}

void SignalLogger::setSignalReceiver(QObject *newSignalReceiver)
{
	if(signalReceiver)
		disconnect(signalReceiver);

	signalReceiver = newSignalReceiver;
	connect(
		this, SIGNAL(newLogMessage(const QString&, const QString&)),
		newSignalReceiver, SLOT(log(const QString&, const QString&))
	);
}
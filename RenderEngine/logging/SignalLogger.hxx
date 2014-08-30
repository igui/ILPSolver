/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "Logger.h"
#include <Qobject>

class SignalLogger: public QObject, public Logger
{
	Q_OBJECT
private:
	QObject *signalReceiver;
public:
	RENDER_ENGINE_EXPORT_API SignalLogger(QObject *parent = 0);

	RENDER_ENGINE_EXPORT_API void log(const char *format, ...);
	RENDER_ENGINE_EXPORT_API void log(const QString &styleOptions, const char *format, ...);
	RENDER_ENGINE_EXPORT_API void vlog(const QString &styleOptions, const char *format, va_list args);

	RENDER_ENGINE_EXPORT_API void setSignalReceiver(QObject *signalReceiver);
signals:
	void newLogMessage(const QString&, const QString&);
};
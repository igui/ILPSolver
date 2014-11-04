#pragma once
#include <QtCore>
#include "ComputeDeviceRepository.h"

class Main : public QObject
{
    Q_OBJECT
public:
    Main(QObject *parent, const QString &filePath, const ComputeDevice &device);
public slots:
    void run();
signals:
    void finished();
private:
	QString filePath;
	ComputeDevice device;
};

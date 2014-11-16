#include "RelPath.h"

#include <QDir>
#include <QCoreApplication>

std::string relativePathToExe(const char *path)
{
	return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(path).toStdString();
}
#include "RelPath.h"

#include <QDir>
#include <QCoreApplication>

std::string relativePathToExe(const std::string &path)
{
	return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(path.c_str()).toStdString();
}
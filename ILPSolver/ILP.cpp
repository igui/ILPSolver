/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "ILP.h"
#include <QDomDocument>
#include <QXmlQuery>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "logging/Logger.h"

ILP::ILP():
	m_scene(NULL)
{
}

ILP ILP::fromFile(Logger *logger, const QString& filePath)
{
	ILP res;

    QFile file(filePath);
	if(!QFileInfo(file).exists())
		throw std::invalid_argument(("File " + filePath + " doesn't exist").toStdString());
    file.open(QFile::ReadOnly);
 
	QXmlQuery query(QXmlQuery::XQuery10);
    query.setFocus(&file);
    query.setQuery("for $scene in /input/scene "
				   "	return string($scene/@path)");
 
    QStringList results;
    query.evaluateTo(&results);
    int count = results.count();
	if(count == 0)
		throw std::invalid_argument("Input must have a scene definition");
    if(count > 1)
		throw std::invalid_argument("Multiple scenes are not allowed in the input");
	

	QString scenePath = results.first();
	QString absoluteScenePath = scenePath;
	if(!QFileInfo(scenePath).isAbsolute())
	{
		absoluteScenePath = QFileInfo(filePath).dir().absoluteFilePath(scenePath);
	}
	Scene *scene = Scene::createFromFile(logger, absoluteScenePath.toStdString().c_str());
	res.m_scene = scene;
	return res;
}
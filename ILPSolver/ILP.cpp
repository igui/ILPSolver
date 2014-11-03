/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "ILP.h"
#include <QDomDocument>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlNodeModelIndex>
#include <QAbstractXmlNodeModel>
#include <qdebug.h>
#include "logging/Logger.h"
#include "LightInSurface.h"
#include "SurfaceRadiosity.h"
#include "renderer/PMOptixRenderer.h"

ILP::ILP():
	scene(NULL),
	optimizationFunction(NULL)
{
}

void ILP::readScene(Logger *logger, QFile &file, const QString& fileName)
{
	QXmlQuery query(QXmlQuery::XQuery10);
    query.setFocus(&file);
    query.setQuery("for $scene in /input/scene "
				   "	return string($scene/@path)");
 
    QStringList results;
	file.reset();
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
		absoluteScenePath = QFileInfo(fileName).dir().absoluteFilePath(scenePath);
	}
	scene = Scene::createFromFile(logger, absoluteScenePath.toStdString().c_str());
}

void ILP::readConditions(Logger *logger, QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i)
	{
		auto conditionParentNode = nodes.at(i).toElement();
		if(conditionParentNode.tagName() != "conditions")
			continue;
		auto conditionNodes = conditionParentNode.childNodes();
		for(int j = 0; j < conditionNodes.length(); ++j)
		{
			auto lightInSurfaceNode = conditionNodes.at(j).toElement();
			if(lightInSurfaceNode.tagName() != "lightInSurface")
				continue;

			QString id = lightInSurfaceNode.attribute("id");
			QString surface = lightInSurfaceNode.attribute("surface");

			if(id.isEmpty())
				throw std::logic_error("id can't be empty");
			if(surface.isEmpty())
				throw std::logic_error("surface can't be empty");

			qDebug() << "condition: LightInSurface id: " + id + ", surface: " + surface;
			conditions.append(new LightInSurface(renderer, scene, id, surface));
		}
	}

	if(conditions.empty())
		throw std::logic_error("at least one condition must be set");
}

void ILP::optimize()
{
	float radius = 0.1f;
	const unsigned int optimizationsRetries = 20;

	for(unsigned int retries = 0; retries < optimizationsRetries; ++retries)
	{
		for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt)
		{
			(*conditionsIt)->pushMoveToNeighbourhood(radius);
		}
		if(optimizationFunction->evaluate() == true)
		{
			retries = -1; // a better solution was found
			continue;
		}
		for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt)
		{
			(*conditionsIt)->popLastMovement();
		}
	}
}

void ILP::readOptimizationFunction(Logger *logger, QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i)
	{
		auto conditionParentNode = nodes.at(i).toElement();
		if(conditionParentNode.tagName() != "objectives")
			continue;
		auto conditionNodes = conditionParentNode.childNodes();
		for(int j = 0; j < conditionNodes.length(); ++j)
		{
			auto maximizeRadianceNode = conditionNodes.at(j).toElement();
			if(maximizeRadianceNode.tagName() != "maximizeRadiance")
				continue;

			if(optimizationFunction != NULL)
				throw std::logic_error("only an objective must be set");


			QString surface = maximizeRadianceNode.attribute("surface");

			if(surface.isEmpty())
				throw std::logic_error("surface can't be empty");

			optimizationFunction = new SurfaceRadiosity(logger, renderer, scene, surface);

			qDebug() << "objective: SurfaceRadiosity on " + surface;
		}
	}

	if(optimizationFunction == NULL)
		throw std::logic_error("an objective must be set");
}

ILP ILP::fromFile(Logger *logger, const QString& filePath, PMOptixRenderer *renderer)
{
	ILP res;

    QFile file(filePath);
	if(!QFileInfo(file).exists())
		throw std::invalid_argument(("File " + filePath + " doesn't exist").toStdString());
    file.open(QFile::ReadOnly);

	QDomDocument xml;
	{
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if(!xml.setContent(&file, &errorMsg, &errorLine, &errorColumn))
		{
			QString msg = "Error while reading file: " + errorMsg;
			throw std::invalid_argument(msg.toStdString());
		}
	}
 
	file.reset();

	res.renderer = renderer;
	res.readScene(logger, file, filePath);
	res.readConditions(logger, xml);
	res.readOptimizationFunction(logger, xml);
	res.renderer->initScene(*res.scene);

	return res;
}
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
#include <ctime>

const QString ILP::logFileName("log.csv");

ILP::ILP():
	scene(NULL),
	optimizationFunction(NULL),
	currentIteration(0),
	logger(NULL),
	renderer(NULL),
	inited(false)
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
	inited = true;
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
	if(!inited){
		throw std::logic_error("ILP is not inited");
	}

	qsrand(time(NULL));

	logIterationHeader();
	auto bestVal = optimizationFunction->evaluate();
	logger->log(QString(), "Initial solution: %s\n", ((QString)*bestVal).toStdString().c_str());
	optimizationFunction->saveImage(getImageFileName());
	logIterationResults(bestVal);

	float radius = 0.05f;
	while(radius < 1.0f){
		auto improvement = findFirstImprovement(bestVal, radius);
		if(improvement != NULL && improvement->better(bestVal)){
			delete bestVal;
			bestVal = improvement;
			radius = 0.05f;
		} else {
			if(improvement != NULL){
				popMoveAll();
			}
			radius += 0.05;
		}
	}
	++currentIteration;	
}

SurfaceRadiosityEvaluation *ILP::findFirstImprovement(SurfaceRadiosityEvaluation *currentEval, float radius)
{
	static const int optimizationsRetries = 20;
	for(int retries = 1; retries < optimizationsRetries; ++retries){
		bool success = pushMoveToNeighbourhoodAll(optimizationsRetries, radius);
		if(!success)
			break; // no neighbours

		auto candidate = optimizationFunction->evaluate();
		optimizationFunction->saveImage(getImageFileName());
		logIterationResults(candidate);
		++currentIteration;

		if(candidate->better(currentEval)){
			logger->log(QString(), "Better solution: %s\n", ((QString)*candidate).toStdString().c_str());
			return candidate; // a better solution was found
		} else {
			delete candidate;
		}
		popMoveAll();
	}
	return NULL; // no better candidate
}

void ILP::popMoveAll()
{
	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt){
		(*conditionsIt)->popLastMovement();
	}
}

bool ILP::pushMoveToNeighbourhoodAll(int optimizationsRetries, float radius)
{
	for(int i = 0; i < conditions.size(); ++i){
		bool hasNeighbour = conditions[i]->pushMoveToNeighbourhood(radius, optimizationsRetries);
		if(!hasNeighbour){
			// the condition has no neighbour. Maybe a limit point or a high radius was reached
			logger->log("No neighbours found\n");
			
			// all or nothing: undoes movements already done
			for(int j = 0; j < i; ++j){
				conditions[j]->popLastMovement();
			}
			return false; // no neighbours
		}
	}
	return true;
}



QString ILP::getImageFileName()
{
	return QString("evaluation-%1.png").arg(currentIteration, 4, 10, QLatin1Char('0'));
}

void ILP::logIterationHeader()
{
	QFile file(logFileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
	QTextStream out(&file);
	
	out << "Iteration" << ';';

	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt)
		out << "Condition " << ((conditionsIt-conditions.cbegin()) + 1)  << ';';
	out << "Optimization Function" << '\n';
	file.close(); 
}


void ILP::logIterationResults(SurfaceRadiosityEvaluation *evaluation)
{
	QFile file(logFileName);
	file.open(QIODevice::Append | QIODevice::Text);
	QTextStream out(&file);
	
	out << currentIteration << ';';

	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt)
		out << (*conditionsIt)->info() << ';';
	out << (*evaluation) << '\n';
	file.close(); 
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

	res.logger = logger;
	res.renderer = renderer;
	res.readScene(logger, file, filePath);
	res.readConditions(logger, xml);
	res.readOptimizationFunction(logger, xml);
	res.renderer->initScene(*res.scene);

	return res;
}
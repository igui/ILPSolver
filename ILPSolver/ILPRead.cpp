#include "ILP.h"
#include <QDomDocument>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QFile>
#include <QFileInfo>
#include <QXmlNodeModelIndex>
#include <QAbstractXmlNodeModel>
#include <QDebug>
#include "conditions/LightInSurface.h"
#include "optimizations/SurfaceRadiosity.h"


void ILP::readScene(QFile &file, const QString& fileName)
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


void ILP::readConditions(QDomDocument& xml)
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


void ILP::readOutputPath(const QString &fileName, QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i){
		auto outputPathNode = nodes.at(i).toElement();
		if(outputPathNode.tagName() != "output")
			continue;

		QString path = outputPathNode.attribute("path");
		if(path.isEmpty()){
			throw std::logic_error("A path attribute must be set for the output element");
		}
		
		outputDir = QDir(QFileInfo(fileName).dir().absoluteFilePath(path));
		return;
	}

	throw std::logic_error("No output path set");
}

void ILP::readOptimizationFunction(QDomDocument& xml)
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
			bool conversionOk;
			float confidenceIntervalRadius = maximizeRadianceNode.attribute("confidenceIntervalRadius").toDouble(&conversionOk);
			if(!conversionOk){
				throw std::logic_error("Invalid confidenceIntervalRadius format");
			}

			if(surface.isEmpty())
				throw std::logic_error("surface can't be empty");

			optimizationFunction = new SurfaceRadiosity(logger, renderer, scene, surface, confidenceIntervalRadius);

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
	res.readScene(file, filePath);
	res.readConditions(xml);
	res.readOptimizationFunction(xml);
	res.renderer->initScene(*res.scene);
	res.readOutputPath(filePath, xml);

	return res;
}

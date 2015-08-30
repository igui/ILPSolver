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
#include "conditions/ObjectInSurface.h"
#include "conditions/DirectionalLight.h"
#include "conditions/ColorCondition.h"
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

static Condition *readLightInSurface(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");
	QString surface = element.attribute("surface");

	if(id.isEmpty())
		throw std::logic_error("id can't be empty");
	if(surface.isEmpty())
		throw std::logic_error("surface can't be empty");

	qDebug("condition: Light in Surface id: %s, surface: %s", qPrintable(id), qPrintable(surface));

	return new LightInSurface(scene, id, surface);
}

static Condition *readObjectInSurface(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");
	QString surface = element.attribute("surface");
	QString surfaceVertexAIndex = element.attribute("vertexAIndex");
	QString surfaceVertexBIndex = element.attribute("vertexBIndex");
	QString surfaceVertexCIndex = element.attribute("vertexCIndex");
	QString surfaceVertexDIndex = element.attribute("vertexDIndex");

	if (id.isEmpty())
		throw std::logic_error("id can't be empty");
	if (surface.isEmpty())
		throw std::logic_error("surface can't be empty");

	if (surfaceVertexAIndex.isEmpty())
		throw std::logic_error("surface A vertex index can't be empty");
	if (surfaceVertexBIndex.isEmpty())
		throw std::logic_error("surface B vertex index can't be empty");
	if (surfaceVertexCIndex.isEmpty())
		throw std::logic_error("surface C vertex index can't be empty");
	if (surfaceVertexDIndex.isEmpty())
		throw std::logic_error("surface D vertex index can't be empty");

	bool ok;
	int vertexAIndex = surfaceVertexAIndex.toInt(&ok);
	if (!ok || vertexAIndex < 0)
		throw std::logic_error("surface A vertex index can't be empty");
	int vertexBIndex = surfaceVertexBIndex.toInt(&ok);
	if (!ok || vertexBIndex < 0)
		throw std::logic_error("surface B vertex index can't be empty");
	int vertexCIndex = surfaceVertexCIndex.toInt(&ok);
	if (!ok || vertexCIndex < 0)
		throw std::logic_error("surface C vertex index can't be empty");
	int vertexDIndex = surfaceVertexDIndex.toInt(&ok);
	if (!ok || vertexDIndex < 0)
		throw std::logic_error("surface D vertex index can't be empty");


	qDebug("condition: Object in Surface id: %s, surface: %s", qPrintable(id), qPrintable(surface));

	return new ObjectInSurface(scene, id, surface, vertexAIndex, vertexBIndex, vertexCIndex, vertexDIndex);
}

static Condition *readDirectionalLight(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");

	if(id.isEmpty())
		throw std::logic_error("id can't be empty");

	qDebug("condition: Directional Light id: %s", qPrintable(id));

	return new DirectionalLight(scene, id);
}

static Condition *readColorCondition(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");

	if (id.isEmpty())
		throw std::logic_error("id can't be empty");

	QString saturationStr = element.attribute("saturation");
	QString valueStr = element.attribute("value");

	if (saturationStr.isEmpty())
		throw std::logic_error("saturation can't be empty");
	if (valueStr.isEmpty())
		throw std::logic_error("value can't be empty");

	bool ok;
	float saturation = saturationStr.toFloat(&ok);
	if (!ok || saturation < 0 || saturation > 1)
		throw std::logic_error("color saturation value is invalid");
	float value = valueStr.toFloat(&ok);
	if (!ok || value < 0 || value > 1)
		throw std::logic_error("color value is invalid");
	
	qDebug("condition: Color Node id: %s", qPrintable(id));

	return new ColorCondition(id, saturation, value);
}

static Condition *readUnknownCondition(Scene *, const QDomElement&)
{
	return NULL;
}

static Condition *readConditionElement(Scene *scene, const QDomElement& element)
{
	auto readFunc = readUnknownCondition;

	if(element.tagName() == "lightInSurface")
		readFunc = readLightInSurface;
	else if (element.tagName() == "objectInSurface")
		readFunc = readObjectInSurface;
	else if(element.tagName() == "directionalLight")
		readFunc = readDirectionalLight;
	else if (element.tagName() == "color")
		readFunc = readColorCondition;
	
	return readFunc(scene, element);
}

void ILP::readConditions(QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i)
	{
		auto conditionParentNode = nodes.at(i).toElement();
		if(conditionParentNode.tagName() != "conditions")
			continue;

		auto meshSizeStr = conditionParentNode.attribute("meshSize");
		if(meshSizeStr.isEmpty())
			throw std::logic_error("meshSize attribute must be present");
		bool ok;
		meshSize = meshSizeStr.toInt(&ok);
		if(!ok || meshSize <= 0)
			throw std::logic_error("meshSize attribute must be a non-negative integer");

		auto conditionNodes = conditionParentNode.childNodes();
		for(int j = 0; j < conditionNodes.length(); ++j)
		{
			auto conditionElement = conditionNodes.at(j).toElement();
			auto condition = readConditionElement(scene, conditionElement);
			if(condition)
				conditions.append(condition);
		}
		break;
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
		auto objectivesNode = nodes.at(i).toElement();
		if(objectivesNode.tagName() != "objectives")
			continue;
		
		QString maxIterationsStr = objectivesNode.attribute("maxIterations");
		if(maxIterationsStr.isEmpty()){
			throw std::logic_error("maxIterations must be present in objectives element");
		}
		bool parseOk;
		maxIterations = maxIterationsStr.toInt(&parseOk);
		if(!parseOk){
			throw std::logic_error("maxIterations must be an integer");
		}

		auto conditionNodes = objectivesNode.childNodes();

		
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

			qDebug("objective: maximize Surface Radiosity on %s", qPrintable(surface));
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

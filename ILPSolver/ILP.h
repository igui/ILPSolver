/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include "Configuration.h"
#include <QVector>
#include <QDir>

class Logger;
class QFile;
class QString;
class Condition;
class OptimizationFunction;
class Evaluation;
class QDomDocument;
class Configuration;

class ILP
{
private:
	static const QString logFileName;
public:
	ILP();
	static ILP fromFile(Logger *logger, const QString& filePath, PMOptixRenderer *renderer);
	void optimize();
private:
	// scene reading
	void readScene(QFile &file, const QString& fileName);
	void readConditions(QDomDocument& doc);
	void readOptimizationFunction(QDomDocument& doc);
	void readOutputPath(const QString &fileName, QDomDocument& doc);

	// misc
	QString getImageFileName();
	QString getImageFileNameSolution(int solutionNum);
	
	// logging
	void cleanOutputDir();
	void logIterationHeader();
	void logIterationResults(QVector<ConditionPosition *>positions, Evaluation *evaluation);

	// optimization
	Configuration processInitialConfiguration();
	bool findFirstImprovement(QVector<Configuration> &currentEvals, float maxRadius, float suffleRadius, int retries);
	QVector<ConditionPosition *> findAllNeighbours(QVector<ConditionPosition *> &currentPositions, int retries, float maxRadius);
	void logBestConfigurations(QVector<Configuration> &bestConfigurations);
	

	bool inited;
	Scene *scene;
	QVector<Condition *> conditions;
	OptimizationFunction *optimizationFunction;
	PMOptixRenderer *renderer;
	int currentIteration;
	int maxIterations;
	Logger *logger;
	QDir outputDir;
};
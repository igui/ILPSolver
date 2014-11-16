/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include <QVector>
#include <QDir>

class Logger;
class QFile;
class QString;
class LightInSurface;
class SurfaceRadiosity;
class SurfaceRadiosityEvaluation;
class QDomDocument;

class ILP
{
private:
	static const QString logFileName;
public:
	ILP();
	static ILP fromFile(Logger *logger, const QString& filePath, PMOptixRenderer *renderer);
	void optimize();
private:
	void readScene(QFile &file, const QString& fileName);
	void readConditions(QDomDocument& doc);
	void readOptimizationFunction(QDomDocument& doc);
	QString getImageFileName();
	void logIterationHeader();
	void logIterationResults(SurfaceRadiosityEvaluation *evaluation);
	SurfaceRadiosityEvaluation *findFirstImprovement(SurfaceRadiosityEvaluation *currentEval, float radius);
	bool pushMoveToNeighbourhoodAll(int retries, float radius);
	void popMoveAll();
	void readOutputPath(const QString &fileName, QDomDocument& doc);
	void cleanOutputDir();

	bool inited;
	Scene *scene;
	QVector<LightInSurface *> conditions;
	SurfaceRadiosity *optimizationFunction;
	PMOptixRenderer *renderer;
	int currentIteration;
	Logger *logger;
	QDir outputDir;
};
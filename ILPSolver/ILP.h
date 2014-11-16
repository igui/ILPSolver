/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include <QVector>

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
	void readScene(Logger *logger, QFile &file, const QString& fileName);
	void readConditions(Logger *logger, QDomDocument& doc);
	void readOptimizationFunction(Logger *logger, QDomDocument& doc);
	QString getImageFileName();
	void logIterationHeader();
	void logIterationResults(SurfaceRadiosityEvaluation *evaluation);
	SurfaceRadiosityEvaluation *findFirstImprovement(SurfaceRadiosityEvaluation *currentEval, float radius);
	bool pushMoveToNeighbourhoodAll(int retries, float radius);
	void popMoveAll();

	bool inited;
	Scene *scene;
	QVector<LightInSurface *> conditions;
	SurfaceRadiosity *optimizationFunction;
	PMOptixRenderer *renderer;
	int currentIteration;
	Logger *logger;
};
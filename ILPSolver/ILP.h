/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include "Configuration.h"
#include "Interval.h"
#include <QVector>
#include <QDir>
#include <QHash>

class Logger;
class QFile;
class QString;
class Condition;
class SurfaceRadiosity;
class SurfaceRadiosityEvaluation;
class QDomDocument;
class Configuration;

uint qHash(const QVector<int> &key, uint seed);

class ILP
{
private:
	static const QString logFileName;
	static const float fastEvaluationQuality;

	struct EvaluateSolutionResult {
		EvaluateSolutionResult();
		
		EvaluateSolutionResult(
			SurfaceRadiosityEvaluation *eval,
			float timeEvaluation
		);
		
		EvaluateSolutionResult(
			bool isCached,
			QVector<int> mappedPositions,
			SurfaceRadiosityEvaluation *eval,
			float timeEvaluation
		);

		bool isCached;
		QVector<int> mappedPositions;
		SurfaceRadiosityEvaluation *evaluation;
		float timeEvaluation;
	};
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
	void logIterationResults(
		QVector<ConditionPosition *>positions,
		SurfaceRadiosityEvaluation *evaluation,
		const QString& iterationComment,
		float evaluationDuration
		);

	// optimization
	Configuration processInitialConfiguration();
	bool findFirstImprovement(float maxRadius, float suffleRadius, int retries);
	bool recalcISOC(
		const QVector<ConditionPosition *>& positions,
		EvaluateSolutionResult evaluation
		);
	QVector<int> getMappedPosition(const QVector<ConditionPosition *>& positions);
	void setEvaluation(
		const QVector<int>& mappedPositions,
		SurfaceRadiosityEvaluation *evaluation
		);
	EvaluateSolutionResult evaluateSolution(const QVector<ConditionPosition *>& positions);
	EvaluateSolutionResult reevalMaxQuality();
	QVector<ConditionPosition *> findAllNeighbours(
		QVector<ConditionPosition *> &currentPositions,
		int retries, float maxRadius
	);
	void logBestConfigurations();

	bool inited;
	Scene *scene;
	QVector<Condition *> conditions;
	int meshSize;
	QHash<QVector<int>, SurfaceRadiosityEvaluation *> evaluations;
	
	QList<Configuration> isoc;
	Interval siIsoc;

	SurfaceRadiosity *optimizationFunction;
	PMOptixRenderer *renderer;
	int currentIteration;
	int maxIterations;
	Logger *logger;
	QDir outputDir;
};
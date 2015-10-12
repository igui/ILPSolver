/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "ILP.h"
#include "logging/Logger.h"
#include "util/sutil.h"
#include "conditions/Condition.h"
#include "conditions/ConditionPosition.h"
#include "optimizations/SurfaceRadiosity.h"
#include "optimizations/SurfaceRadiosityEvaluation.h"
#include "Configuration.h"
#include <algorithm>
#include <iterator>     
#include <ctime>
#include <qDebug>
#include <QtConcurrent/QtConcurrentFilter>
#include <QtConcurrent/QtConcurrentMap>

const float ILP::fastEvaluationQuality = 0.25f;

ILP::ILP():
	scene(NULL),
	optimizationFunction(NULL),
	currentIteration(0),
	logger(NULL),
	renderer(NULL),
	inited(false),
	siIsoc(0, 0)
{
	statistics = { 0 };
}

uint qHash(const QVector<int> &key, uint seed)
{
  std::size_t ret = 0;
  for(auto& i : key) {
    ret ^= i + 0x9e3779b9 + (ret << 6) + (ret >> 2);
  }
  return ret ^ seed;
}

static int getRetriesForRadius(float radius)
{
	int res = ceilf(sqrtf(radius) * 200.0f);
	return res;
}

static float getShuffleRadius(float progress)
{
	return 0.5f * (0.5f + sinf(2.0f * (float)M_PI * progress - 0.5f * (float)M_PI) / 2.0f);
}

void ILP::optimize()
{
	if(!inited){
		throw std::logic_error("ILP is not inited");
	}

	qsrand(std::time(NULL));
	
	double start =  sutilCurrentTime();

	// first solution
	processInitialConfiguration();

	// apply ILP
	while(currentIteration < maxIterations)
	{
		float radius = 0.05f;
		while(radius < 1.0f){
			float suffleRadius = getShuffleRadius(
				(float) currentIteration / maxIterations
			);
			bool improvementFound = findFirstImprovement(
				radius,
				suffleRadius,
				getRetriesForRadius(radius)
				);
			if(improvementFound){
				radius = 0.05f;
			} else {
				radius += 0.05;
			}
		}
		logger->log("Done navigating whole neighbourhood: %d\n", currentIteration);
	}
	double totalTime = sutilCurrentTime() - start;
	statistics.totalTime = totalTime;
	
	logger->log("%d iterations on %0.2fs. %0.2fs per iteration\n", 
		(currentIteration+1), totalTime, totalTime/(currentIteration+1));

	logStatistics();

	logBestConfigurations();
}

Configuration ILP::processInitialConfiguration()
{
	logIterationHeader();
	auto initialEval = reevalMaxQuality();
	auto initialPositions = QtConcurrent::blockingMapped(
		conditions,
		(ConditionPosition *(*)(Condition *)) [] (Condition * condition) { return condition->initial(); }
	);
		
	auto initialConfig = Configuration(initialEval.evaluation, initialPositions);
	isoc.clear();

	QString initialIterationComment;
	
	if (initialEval.evaluation->valid())
	{
		isoc.append(initialConfig);
		siIsoc = initialEval.evaluation->interval();
		initialIterationComment = "INITIAL";
	}
	else
	{
		isoc.append(initialConfig);
		siIsoc = Interval(0, 0);
		initialIterationComment = "INITIAL INVALID";
	}


	logger->log("Initial  solution: %s\n", qPrintable(initialEval.evaluation->infoShort()));
	logIterationResults(
		initialConfig.positions(),
		initialEval.evaluation,
		initialIterationComment,
		initialEval.timeEvaluation
	);
	++currentIteration;
	return initialConfig;
}

bool ILP::recalcISOC(
	const QVector<ConditionPosition *> &positions,
	ILP::EvaluateSolutionResult eval)
{
	if(eval.isCached) {
		++currentIteration;
		return false;
	}

	if (!eval.evaluation->valid())
	{
		logIterationResults(positions, eval.evaluation, "INVALID", eval.timeEvaluation);
		++currentIteration;
		return false;
	}

	if(eval.evaluation->interval() < siIsoc) {
		++currentIteration;
		logIterationResults(positions, eval.evaluation, "BAD", eval.timeEvaluation);

		// removes positions from memory
		for(auto p: positions){
			delete p;
		}
		return false;
	} 

	// evaluation is may belong to isoc it's reevaluated with max quality
	
	if(!eval.evaluation->isMaxQuality()) {
		delete eval.evaluation;
		auto reevaluatedSolution = reevalMaxQuality();
		setEvaluation(eval.mappedPositions, reevaluatedSolution.evaluation);

		eval = reevaluatedSolution;
	}

	if(eval.evaluation->interval() < siIsoc) {
		logIterationResults(positions, eval.evaluation, "BAD-AFTER-REEVAL", eval.timeEvaluation);
		// removes evaluation from memory
		for(auto p: positions){
			delete p;
		}
		return false;
	} 
	
	if(eval.evaluation->interval() > siIsoc) {
		logger->log("Better   solution: %s\n", qPrintable(eval.evaluation->infoShort()));
		logIterationResults(positions, eval.evaluation, "BETTER", eval.timeEvaluation);

		QtConcurrent::blockingFilter(isoc, [eval](Configuration config){
			return config.evaluation()->interval().intersects(eval.evaluation->interval());
		});

		auto newSiIsoc = eval.evaluation->interval();
		for(auto configuration: isoc){
			newSiIsoc = newSiIsoc.intersection(configuration.evaluation()->interval());
		}
		siIsoc = newSiIsoc;

		isoc.append(Configuration(eval.evaluation, positions));

		return true;
	}
	else {
		logger->log("Probable solution: %s\n", qPrintable(eval.evaluation->infoShort()));
		logIterationResults(positions, eval.evaluation, "PROBABLE", eval.timeEvaluation);

		// recalculate ISOC and SIISOC
		auto newSiIsoc = eval.evaluation->interval();
		for(auto configuration: isoc){
			newSiIsoc = newSiIsoc.intersection(configuration.evaluation()->interval());
		}
		siIsoc = newSiIsoc;
		isoc.append(Configuration(eval.evaluation, positions));
	}
	return false;
}

bool ILP::findFirstImprovement(float maxRadius, float shuffleRadius, int retries)
{
	static const int neighbourhoodRetries = 20;

	while(retries-- > 0){
		// move the reference point to some element of the configuration file
		int someConfigIdx = rand() % isoc.length();
		auto positions = isoc.at(someConfigIdx).positions();

		// shuffle condition positions a bit
		int shuffled;
		for(shuffled = 0; shuffled < positions.size(); ++shuffled){
			auto currentShuffleRadius = shuffleRadius * qrand() / RAND_MAX;
			auto neighbour = conditions.at(shuffled)->findNeighbour(
				positions.at(shuffled), currentShuffleRadius, neighbourhoodRetries
			);
			if(!neighbour){
				break; // can't shuffle
			}
			positions[shuffled] = neighbour;
		}
		if(shuffled < positions.size()){
			continue; // can't shuffle
		}
		
		// find neighbours
		auto neighbourPosition = findAllNeighbours(positions, neighbourhoodRetries, maxRadius);
		if(neighbourPosition.empty()){
			continue; // no neighbours
		}

		auto eval = evaluateSolution(neighbourPosition);
		bool isImprovement = recalcISOC(neighbourPosition, eval);
		if(isImprovement)
		{
			return true;
		}
	}
	return false;
}

QVector<int> ILP::getMappedPosition(const QVector<ConditionPosition *>& positions)
{
	// builds a mapped positions using relative dimension size
	QVector<int> mappedPositions;
	for(int conditionIdx = 0; conditionIdx < conditions.length(); ++conditionIdx) {
		auto dimensions = conditions[conditionIdx]->dimensions();
		for(int dimensionIdx = 0; dimensionIdx < dimensions.length(); ++dimensionIdx) {
			auto normalizedPosition = positions[conditionIdx]->normalizedPosition();
			mappedPositions.append(
				floorf(meshSize * normalizedPosition[dimensionIdx])
			);
		}
	}
	
	/*QStringList mappedPositionsStr;
	std::transform(
		mappedPositions.begin(),
		mappedPositions.end(),
		std::back_inserter(mappedPositionsStr),
		[](const int& x) { return QString::number(x); }
	);
	qDebug() << "Mapped positions: " << mappedPositionsStr.join(", ") << "\n";*/

	return mappedPositions;
}


ILP::EvaluateSolutionResult ILP::evaluateSolution(const QVector<ConditionPosition *>& positions)
{
	double startTime = sutilCurrentTime();
	auto mappedPositions = getMappedPosition(positions);
	auto evaluation = evaluations[mappedPositions];
	if(evaluation)
	{
		//qDebug() << "Returning saved evaluation for " << mappedPositionsStr.join(", ") << ": " << evaluation->infoShort() << "\n";
		return EvaluateSolutionResult(true, mappedPositions, evaluation, sutilCurrentTime() - startTime);
	}
	else
	{
		//qDebug() << "Calculating evaluation for " << mappedPositionsStr.join(", ") << "\n";

		for(auto position: positions) {
			position->apply(renderer);
		}

		// TODO use evaluate fast
		auto candidate = optimizationFunction->evaluateFast(fastEvaluationQuality);
		/*auto candidate = optimizationFunction->evaluateRadiosity();
		auto imagePath = outputDir.filePath(
			QString("solution-%1.png").arg(currentIteration, 4, 10, QLatin1Char('0'))
		);
		optimizationFunction->saveImage(imagePath);*/
		
		evaluations[mappedPositions] = candidate;

		evaluation = candidate;

		auto totalTime = sutilCurrentTime() - startTime;
		statistics.evaluationTime += totalTime;
		statistics.evaluations++;
		return EvaluateSolutionResult(false, mappedPositions, evaluation, totalTime);
	}
}

void ILP::setEvaluation(const QVector<int>& mappedPositions, SurfaceRadiosityEvaluation *evaluation)
{
	evaluations[mappedPositions] = evaluation;
}

ILP::EvaluateSolutionResult ILP::reevalMaxQuality()
{
	double startTime = sutilCurrentTime();
	auto evaluation = optimizationFunction->evaluateFast(1.0f);
	auto totalTime = sutilCurrentTime() - startTime;
	statistics.evaluationTime += totalTime;
	statistics.evaluations++;
	return EvaluateSolutionResult(evaluation, totalTime);
}

QVector<ConditionPosition *> ILP::findAllNeighbours(
	QVector<ConditionPosition *> &currentPositions,
	int optimizationsRetries, 
	float maxRadius
)
{
	QVector<ConditionPosition *> res;

	float radius = maxRadius * qrand() / RAND_MAX;

	// generate partitions
	auto corradius = QVector<float>() << 0.0f;
	for(int i = 0; i < conditions.size()-1; ++i){
		corradius.append(radius * qrand() / RAND_MAX);
	}
	corradius << radius;
	qSort(corradius);

	for(int i = 0; i < conditions.size(); ++i){
		auto neighbourRadius = corradius.at(i+1) - corradius.at(i);
		auto neighbour = conditions.at(i)->findNeighbour(
			currentPositions.at(i),
			neighbourRadius,
			optimizationsRetries
		);
		if(neighbour == NULL){
			for(auto r: res){
				delete r;
			}
			res.clear();
			return res; // empty array: no neighbours
		} else {
			res.append(neighbour);
		}
	}
	return res;
}

QString ILP::getImageFileName()
{
	return  outputDir.filePath(
		QString("evaluation-%1.png").arg(currentIteration, 4, 10, QLatin1Char('0'))
	);
}

QString ILP::getImageFileNameSolution(int solutionNum)
{
	return  outputDir.filePath(
		QString("evaluation-solution-%1.png").arg(solutionNum, 4, 10, QLatin1Char('0'))
	);
}

ILP::EvaluateSolutionResult::EvaluateSolutionResult(
	bool isCached,
	QVector<int> mappedPositions,
	SurfaceRadiosityEvaluation *eval,
	float timeEvaluation
):
	isCached(isCached),
	mappedPositions(mappedPositions),
	evaluation(eval),
	timeEvaluation(timeEvaluation)
{
}

ILP::EvaluateSolutionResult::EvaluateSolutionResult(
	SurfaceRadiosityEvaluation *eval,
	float timeEvaluation
):
	isCached(false),
	evaluation(eval),
	timeEvaluation(timeEvaluation)
{
}

ILP::EvaluateSolutionResult::EvaluateSolutionResult():
	isCached(false),
	evaluation(NULL),
	timeEvaluation(0)
{
}

/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Problem.h"
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

Problem::Problem():
	scene(NULL),
	optimizationFunction(NULL),
	currentIteration(0),
	logger(NULL),
	renderer(NULL),
	inited(false),
	siIsoc(0, 0),
	startTime(0)
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

void Problem::optimize()
{
	if(!inited){
		throw std::logic_error("Problem is not inited");
	}

	qsrand(std::time(NULL));
	
	startTime = sutilCurrentTime();

	// first solution
	processInitialConfiguration();

	// apply Problem
	while(currentIteration < maxIterations)
	{
		float radius = 0.05f;
		while(radius < 1.0f && currentIteration < maxIterations)
		{
			float suffleRadius = getShuffleRadius(
				(float) currentIteration / maxIterations
			);
			bool improvementFound = findFirstImprovement(
				radius,
				suffleRadius,
				getRetriesForRadius(radius)
				);
			if(improvementFound)
			{
				radius = 0.05f;
			} else {
				radius += 0.05;
			}
		}
		logger->log("Done navigating whole neighbourhood: %d\n", currentIteration);
	}

	finishingISOCRefinement();

	double totalTime = 
		statistics.totalTime = sutilCurrentTime() - startTime;;
	
	logger->log("%d iterations on %0.2fs. %0.2fs per iteration\n", 
		(currentIteration+1), totalTime, totalTime/(currentIteration+1));

	logStatistics();

	logBestConfigurations();
}

void Problem::finishingISOCRefinement()
{
	if (strategy != REFINE_ISOC_ON_END)
	{
		return;
	}

	logger->log("Finishing ISOC Refinement\n");

	struct PositionEvalResult {
		QVector<ConditionPosition *> positions;
		EvaluateSolutionResult evalResult;
	};

	QList<PositionEvalResult> maxQualityISOCCandidates;
	for (auto conf : isoc)
	{
		double startTime = sutilCurrentTime();
		for (auto position : conf.positions()) {
			position->apply(renderer);
		}

		auto evaluation = optimizationFunction->evaluateFast(1.0f);
		auto totalTime = sutilCurrentTime() - startTime;
		statistics.evaluationTime += totalTime;
		statistics.evaluations++;
	

		maxQualityISOCCandidates.append(PositionEvalResult{
			conf.positions(),
			EvaluateSolutionResult(evaluation, totalTime)
		});
	}

	isoc.clear();
	siIsoc = Interval(0, 0);

	for (auto candidate : maxQualityISOCCandidates)
	{
		recalcISOC(candidate.positions, candidate.evalResult);
	}
}

float Problem::initialConfigurationQuality()
{
	switch (strategy)
	{
	case REFINE_ISOC_ON_INTERSECTION:
		return 1.0f;
	case REFINE_ISOC_ON_END:
	case NO_REFINE_ISOC:
		return fastEvaluationQuality;
	default:
		throw std::logic_error("No such strategy");
		break;
	}
}

float Problem::evalConfigurationQuality()
{
	switch (strategy)
	{
	case REFINE_ISOC_ON_INTERSECTION:
	case REFINE_ISOC_ON_END:
	case NO_REFINE_ISOC:
		return fastEvaluationQuality;
	default:
		throw std::logic_error("No such strategy");
		break;
	}
}

Configuration Problem::processInitialConfiguration()
{
	logIterationHeader();
	
	double startTime = sutilCurrentTime();
	auto evaluation = optimizationFunction->evaluateFast(initialConfigurationQuality());
	auto totalTime = sutilCurrentTime() - startTime;
	statistics.evaluationTime += totalTime;
	statistics.evaluations++;
	auto initialEval = EvaluateSolutionResult(evaluation, totalTime);


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

static void cleanPositionsFromMemory(const QVector<ConditionPosition *> &positions)
{
	for (auto p : positions){
		delete p;
	}
}

bool Problem::recalcISOC(
	const QVector<ConditionPosition *> &positions,
	Problem::EvaluateSolutionResult eval)
{
	if (eval.isCached) {
		++currentIteration;
		return false;
	}

	if (!eval.evaluation->valid())
	{
		logIterationResults(positions, eval.evaluation, "INVALID", eval.timeEvaluation);
		++currentIteration;
		return false;
	}

	if (eval.evaluation->interval() < siIsoc) {
		++currentIteration;
		logIterationResults(positions, eval.evaluation, "BAD", eval.timeEvaluation);

		cleanPositionsFromMemory(positions);
		return false;
	}

	// evaluation is may belong to isoc it's reevaluated with max quality
	if (strategy == REFINE_ISOC_ON_INTERSECTION)
	{
		if (!eval.evaluation->isMaxQuality()) {
			delete eval.evaluation;
			auto reevaluatedSolution = reevalMaxQuality();
			setEvaluation(eval.mappedPositions, reevaluatedSolution.evaluation);

			eval = reevaluatedSolution;
		}

		if (eval.evaluation->interval() < siIsoc) {
			logIterationResults(positions, eval.evaluation, "BAD-AFTER-REEVAL", eval.timeEvaluation);
			cleanPositionsFromMemory(positions);
			return false;
		}
	}

	bool isImprovement = false;
	QList<Configuration> newIsoc;
	for (auto configuration : isoc)
	{
		switch (eval.evaluation->compareWith(configuration.evaluation()))
		{
		case SurfaceRadiosityEvaluation::WORSE:
			logIterationResults(positions, eval.evaluation, "BAD-AFTER-REEVAL-ISOC", eval.timeEvaluation);
			cleanPositionsFromMemory(positions);
			return false;
		case SurfaceRadiosityEvaluation::INDISTINGUISHABLE:
			newIsoc.append(configuration);
			break;
		case SurfaceRadiosityEvaluation::BETTER:
			isImprovement = true;
			cleanPositionsFromMemory(configuration.positions());
			break;
		}
	}
	isoc = newIsoc;
	isoc.append(Configuration(eval.evaluation, positions));
	
	siIsoc = eval.evaluation->interval();
	
	for (auto configuration : isoc){
		siIsoc = siIsoc.intersection(configuration.evaluation()->interval());
	}
	logger->log("Probable solution: %s. ISOC size %d\n", qPrintable(eval.evaluation->infoShort()), isoc.length());


	auto iterationResultComment = isImprovement ? "BETTER" : "PROBABLE";
	logIterationResults(positions, eval.evaluation, iterationResultComment, eval.timeEvaluation);

	return isImprovement;
}

bool Problem::findFirstImprovement(float maxRadius, float shuffleRadius, int retries)
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

QVector<int> Problem::getMappedPosition(const QVector<ConditionPosition *>& positions)
{
	// builds a mapped positions using relative dimension size
	QVector<int> mappedPositions;
	for(int conditionIdx = 0; conditionIdx < conditions.length(); ++conditionIdx) {
		auto dimensions = conditions[conditionIdx]->dimensions();
		for(int dimensionIdx = 0; dimensionIdx < dimensions.length(); ++dimensionIdx) {
			auto normalizedPosition = positions[conditionIdx]->normalizedPosition();
			auto meshSizeForDimension = meshSize.at(dimensionIdx % meshSize.length());
			mappedPositions.append(
				floorf(meshSizeForDimension * normalizedPosition[dimensionIdx])
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


Problem::EvaluateSolutionResult Problem::evaluateSolution(const QVector<ConditionPosition *>& positions)
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
		/*auto candidate = optimizationFunction->evaluateFast(evalConfigurationQuality());*/
		auto candidate = optimizationFunction->evaluateRadiosity();
		auto imagePath = outputDir.filePath(
			QString("solution-%1.png").arg(currentIteration, 4, 10, QLatin1Char('0'))
		);
		optimizationFunction->saveImage(imagePath);
		
		evaluations[mappedPositions] = candidate;

		evaluation = candidate;

		auto totalTime = sutilCurrentTime() - startTime;
		statistics.evaluationTime += totalTime;
		statistics.evaluations++;
		return EvaluateSolutionResult(false, mappedPositions, evaluation, totalTime);
	}
}

void Problem::setEvaluation(const QVector<int>& mappedPositions, SurfaceRadiosityEvaluation *evaluation)
{
	evaluations[mappedPositions] = evaluation;
}

Problem::EvaluateSolutionResult Problem::reevalMaxQuality()
{
	double startTime = sutilCurrentTime();
	auto evaluation = optimizationFunction->evaluateFast(1.0f);
	auto totalTime = sutilCurrentTime() - startTime;
	statistics.evaluationTime += totalTime;
	statistics.evaluations++;
	return EvaluateSolutionResult(evaluation, totalTime);
}

QVector<ConditionPosition *> Problem::findAllNeighbours(
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

QString Problem::getImageFileName()
{
	return  outputDir.filePath(
		QString("evaluation-%1.png").arg(currentIteration, 4, 10, QLatin1Char('0'))
	);
}

QString Problem::getImageFileNameSolution(int solutionNum)
{
	return  outputDir.filePath(
		QString("evaluation-solution-%1.png").arg(solutionNum, 4, 10, QLatin1Char('0'))
	);
}

Problem::EvaluateSolutionResult::EvaluateSolutionResult(
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

Problem::EvaluateSolutionResult::EvaluateSolutionResult(
	SurfaceRadiosityEvaluation *eval,
	float timeEvaluation
):
	isCached(false),
	evaluation(eval),
	timeEvaluation(timeEvaluation)
{
}

Problem::EvaluateSolutionResult::EvaluateSolutionResult():
	isCached(false),
	evaluation(NULL),
	timeEvaluation(0)
{
}

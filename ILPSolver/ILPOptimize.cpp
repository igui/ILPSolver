/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "ILP.h"
#include "logging/Logger.h"
#include "conditions/Condition.h"
#include "conditions/ConditionPosition.h"
#include "optimizations/OptimizationFunction.h"
#include "optimizations/Evaluation.h"
#include "Configuration.h"
#include <ctime>

const QString ILP::logFileName("solutions.csv");

ILP::ILP():
	scene(NULL),
	optimizationFunction(NULL),
	currentIteration(0),
	logger(NULL),
	renderer(NULL),
	inited(false)
{
}

void ILP::optimize()
{
	if(!inited){
		throw std::logic_error("ILP is not inited");
	}

	qsrand(time(NULL));

	// first solution
	QVector<Configuration> bestConfigurations;
	bestConfigurations.append(processInitialConfiguration());

	// apply ILP
	float radius = 0.05f;
	while(radius < 1.0f){
		findFirstImprovement(bestConfigurations, radius);
		if(bestConfigurations.length() == 1){
			radius = 0.05f;
		} else {
			radius += 0.05;
		}
	}

	logBestConfigurations(bestConfigurations);
}

void ILP::logBestConfigurations(QVector<Configuration> &bestConfigurations)
{
	logger->log("Best values(%d)\n", bestConfigurations.length());
	for(auto it = bestConfigurations.begin(); it != bestConfigurations.end(); ++it){
		auto positions = it->positions();
		for(auto positionsIt = positions.begin(); positionsIt != positions.end(); ++positionsIt){
			(*positionsIt)->apply(renderer);
		}
		auto evalSolutionNow = optimizationFunction->evaluateRadiosity();
		optimizationFunction->saveImage(getImageFileNameSolution(it - bestConfigurations.begin()));
		QString evalInfo = *it->evaluation();
		QString evalInfoNow = *evalSolutionNow;
		logger->log(
			"\tEval: %s (now evals as %s)\n",
			evalInfo.toStdString().c_str(),
			evalInfoNow.toStdString().c_str()
		);
	}
}

Configuration ILP::processInitialConfiguration()
{
	logIterationHeader();
	auto initialEval = optimizationFunction->evaluateFast();
	auto initialConfig = Configuration(initialEval, QVector<ConditionPosition *>());
	logger->log("Initial solution: %s\n", ((QString)(*initialEval)).toStdString().c_str());
	logIterationResults(initialEval);
	++currentIteration;
	return initialConfig;
}

static bool appendAndRemoveWorse(QVector<Configuration> &currentEvals, Evaluation *candidate, QVector<ConditionPosition *> &positions)
{
	bool candidateIsGoodEnough = false;
	auto it = currentEvals.begin();
	while(it != currentEvals.end()){
		auto comp = candidate->compare(it->evaluation());
		if(comp == EvaluationResult::BETTER){
			it = currentEvals.erase(it);
		} else {
			++it;
		}
		candidateIsGoodEnough = candidateIsGoodEnough || (comp != EvaluationResult::WORSE);
	}
	
	if(candidateIsGoodEnough){
		currentEvals.append(Configuration(candidate, positions));
	}
	return candidateIsGoodEnough;
}

void ILP::findFirstImprovement(QVector<Configuration> &configurations, float radius)
{
	static const int optimizationsRetries = 20;
	for(int retries = 1; retries < optimizationsRetries; ++retries){
		auto neighbourPosition = pushMoveToNeighbourhoodAll(optimizationsRetries, radius);
		if(neighbourPosition.empty()){
			break; // no neighbours
		}

		auto candidate = optimizationFunction->evaluateFast();
		//optimizationFunction->saveImage(getImageFileName());
		logIterationResults(candidate);
		++currentIteration;

		bool isGoodEnough = appendAndRemoveWorse(configurations, candidate, neighbourPosition);

		if(isGoodEnough && configurations.length() == 1){
			logger->log("Better solution: %s\n", ((QString)*candidate).toStdString().c_str());
			break; 
		} else {	
			if(!isGoodEnough) {
				// removes candidate and positions from memory
				delete candidate;
				for(auto it = neighbourPosition.begin(); it != neighbourPosition.end(); ++it){
					delete *it;
				}
			} else {
				logger->log("Probable solution: %s\n", ((QString)*candidate).toStdString().c_str());
			}
			popMoveAll();
		}
	}
}

void ILP::popMoveAll()
{
	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt){
		(*conditionsIt)->popLastMovement();
	}
}

QVector<ConditionPosition *> ILP::pushMoveToNeighbourhoodAll(int optimizationsRetries, float radius)
{
	QVector<ConditionPosition *> res;

	for(int i = 0; i < conditions.size(); ++i){
		auto neighbour = conditions[i]->pushMoveToNeighbourhood(radius, optimizationsRetries);
		if(neighbour == NULL){
			// the condition has no neighbour. Maybe a limit point or a high radius was reached
			logger->log("No neighbours found\n");
			
			// all or nothing: undoes movements already done
			for(int j = 0; j < i; ++j){
				conditions[j]->popLastMovement();
			}

			for(auto it = res.begin(); it != res.end(); ++it){
				delete *it;
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

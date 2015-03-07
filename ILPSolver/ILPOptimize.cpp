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
#include "optimizations/OptimizationFunction.h"
#include "optimizations/Evaluation.h"
#include "Configuration.h"
#include <ctime>

ILP::ILP():
	scene(NULL),
	optimizationFunction(NULL),
	currentIteration(0),
	logger(NULL),
	renderer(NULL),
	inited(false)
{
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

	qsrand(time(NULL));

	// first solution
	QVector<Configuration> bestConfigurations;
	bestConfigurations.append(processInitialConfiguration());

	double start = sutilCurrentTime();

	// apply ILP
	while(currentIteration < maxIterations)
	{
		float radius = 0.05f;
		while(radius < 1.0f){
			float suffleRadius = getShuffleRadius((float) currentIteration / maxIterations);
			bool improvementFound = findFirstImprovement(bestConfigurations, radius,  suffleRadius, getRetriesForRadius(radius));
			if(improvementFound){
				radius = 0.05f;
			} else {
				radius += 0.05;
			}
		}
		logger->log("Done navigating whole neighbourhood: %d\n", currentIteration);
	}
	double totalTime = sutilCurrentTime() - start;
	
	logger->log("%d iterations on %0.2fs. %0.2fs per iteration\n", 
		(currentIteration+1), totalTime, totalTime/(currentIteration+1));

	logBestConfigurations(bestConfigurations);
}

Configuration ILP::processInitialConfiguration()
{
	logIterationHeader();
	auto initialEval = optimizationFunction->evaluateFast();
	auto initialPositions = QVector<ConditionPosition *>();
	for(auto conditionIt = conditions.begin(); conditionIt != conditions.end(); ++conditionIt){
		initialPositions.append((*conditionIt)->initial());
	}
	auto initialConfig = Configuration(initialEval, initialPositions);
	logger->log("Initial solution: %s\n", initialEval->infoShort().toStdString().c_str() );
	logIterationResults(initialConfig.positions(), initialEval);
	++currentIteration;
	return initialConfig;
}

static bool appendAndRemoveWorse(QVector<Configuration> &currentEvals, Evaluation *candidate, QVector<ConditionPosition *> &positions)
{
	QVector<Configuration> evals = currentEvals;
	currentEvals.clear();

	bool candidateIsGoodEnough = false;
	for(auto i = 0; i < evals.size(); ++i){
		auto comp = candidate->compare(evals.at(i).evaluation());
		if(comp != EvaluationResult::BETTER){
			currentEvals.append(evals.at(i));
		}			

		if(!candidateIsGoodEnough && comp != EvaluationResult::WORSE){
			candidateIsGoodEnough = true;
			currentEvals.append(Configuration(candidate, positions));
		}
	}
	return candidateIsGoodEnough;
}

bool ILP::findFirstImprovement(QVector<Configuration> &configurations, float maxRadius, float shuffleRadius, int retries)
{
	static const int neighbourhoodRetries = 20;

	while(retries-- > 0){
		// move the reference point to some element of the configuration file
		int someConfigIdx = rand() % configurations.length();
		auto positions = configurations.at(someConfigIdx).positions();

		// shuffle condition positions a bit
		int shuffled;
		for(shuffled = 0; shuffled < positions.size(); ++shuffled){
			auto currentShuffleRadius = shuffleRadius * qrand() / RAND_MAX;
			auto neighbour = conditions.at(shuffled)->findNeighbour(positions.at(shuffled), currentShuffleRadius, retries);
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

		// evaluate function
		for(auto it = neighbourPosition.begin(); it != neighbourPosition.end(); ++it){
			(*it)->apply(renderer);
		}
		auto candidate = optimizationFunction->evaluateFast();
		logIterationResults(positions, candidate);
		++currentIteration;
		
		// crop worse solutions
		bool isGoodEnough = appendAndRemoveWorse(configurations, candidate, neighbourPosition);

		// evaluate if the solution is an improvement
		if(isGoodEnough && configurations.length() == 1){
			logger->log("Better solution: %s\n", candidate->infoShort().toStdString().c_str());
			return true; 
		} else {	
			if(!isGoodEnough) {
				// removes candidate and positions from memory
				delete candidate;
				for(auto it = neighbourPosition.begin(); it != neighbourPosition.end(); ++it){
					delete *it;
				}
			} else {
				logger->log("Probable solution: %s\n", candidate->infoShort().toStdString().c_str());
			}
		}
	}
	return false;
}

QVector<ConditionPosition *> ILP::findAllNeighbours(QVector<ConditionPosition *> &currentPositions, int optimizationsRetries,  float maxRadius)
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
		auto neighbour = conditions.at(i)->findNeighbour(currentPositions.at(i), neighbourRadius, optimizationsRetries);
		if(neighbour == NULL){
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

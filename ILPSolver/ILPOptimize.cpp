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
		float minRadius = 0.0f;
		float maxRadius = 0.05f;
		while(maxRadius < 1.0f){
			float suffleRadius = getShuffleRadius((float) currentIteration / maxIterations);
			bool improvementFound = findFirstImprovement(bestConfigurations, minRadius, maxRadius,  suffleRadius, getRetriesForRadius(maxRadius));
			if(improvementFound){
				minRadius = 0;
				maxRadius = 0.05f;
			} else {
				minRadius = maxRadius;
				maxRadius += 0.05;
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

bool ILP::findFirstImprovement(QVector<Configuration> &configurations, float minRadius, float maxRadius, float shuffleRadius, int retries)
{
	static const int neighbourhoodRetries = 20;

	while(retries-- > 0){
		// move the reference point to some element of the configuration file
		int someConfigIdx = rand() % configurations.length();
		auto positions = configurations.at(someConfigIdx).positions();

		// shuffle condition positions a bit
		for(int i = 0; i < positions.size(); ++i){
			auto currentShuffleRadius = shuffleRadius * qrand() / RAND_MAX;
			auto neighbour = conditions.at(i)->findNeighbour(positions.at(i), currentShuffleRadius, retries);
			if(!neighbour){
				return false; // couldn't suffle
			}
			positions[i] = neighbour;
		}
		
		// find neighbours
		auto neighbourPosition = findAllNeighbours(positions, neighbourhoodRetries, minRadius, maxRadius);
		if(neighbourPosition.empty()){
			return false; // no neighbours
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

			/*for(int i = 0; i < 10; ++i)
			{
				auto candidateReeval = optimizationFunction->evaluateFast();
				logger->log("                 %s (%s)\n",
					candidateReeval->infoShort().toStdString().c_str(),
					toString(candidateReeval->compare(candidate)).toStdString().c_str());
			}*/

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
				/*for(int i = 0; i < 10; ++i)
				{
					auto candidateReeval = optimizationFunction->evaluateFast();
					logger->log("                   %s (%s)\n",
						candidateReeval->infoShort().toStdString().c_str(),
						toString(candidateReeval->compare(candidate)).toStdString().c_str());
				}*/
			}
		}
	}
	return false;
}

QVector<ConditionPosition *> ILP::findAllNeighbours(QVector<ConditionPosition *> &currentPositions, int optimizationsRetries, float minRadius, float maxRadius)
{
	QVector<ConditionPosition *> res;

	float radius = ((maxRadius - minRadius) * qrand()) / RAND_MAX + minRadius;

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
			// the condition has no neighbour. Maybe a limit point or a high radius was reached
			logger->log("No neighbours found (radius %0.2f to %0.2f)\n", minRadius, maxRadius);
			
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

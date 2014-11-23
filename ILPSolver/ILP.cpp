/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "ILP.h"
#include "logging/Logger.h"
#include "conditions/Condition.h"
#include "optimizations/Evaluation.h"
#include "optimizations/OptimizationFunction.h"
#include "Configuration.h"
#include <QTextStream>
#include <ctime>

const QString ILP::logFileName("log.csv");

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
	logIterationHeader();
	QVector<Configuration> bestSolutions;
	auto initialEval = optimizationFunction->evaluateFast();
	auto initialConfig = Configuration(initialEval, QVector<ConditionPosition *>());
	bestSolutions.append(initialConfig);
	logger->log(QString(), "Initial solution: %s\n", ((QString)(*initialEval)).toStdString().c_str());
	//optimizationFunction->saveImage(getImageFileName());
	logIterationResults(initialEval);
	++currentIteration;

	float radius = 0.05f;
	while(radius < 1.0f){
		findFirstImprovement(bestSolutions, radius);
		if(bestSolutions.length() == 1){
			radius = 0.05f;
		} else {
			radius += 0.05;
		}
	}

	logger->log(QString(), "Best values(%d)\n", bestSolutions.length());
	for(auto it = bestSolutions.begin(); it != bestSolutions.end(); ++it){
		QString evalInfo = *it->evaluation();
		logger->log(QString(), "\t%s\n", evalInfo.toStdString().c_str());
	}

	++currentIteration;	
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
			logger->log(QString(), "Better solution: %s\n", ((QString)*candidate).toStdString().c_str());
			break; 
		} else {	
			if(!isGoodEnough) {
				// removes candidate and positions from memory
				delete candidate;
				for(auto it = neighbourPosition.begin(); it != neighbourPosition.end(); ++it){
					delete *it;
				}
			} else {
				logger->log(QString(), "Probable solution: %s\n", ((QString)*candidate).toStdString().c_str());
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

void ILP::cleanOutputDir()
{
	if(!outputDir.exists()){
		outputDir.mkpath(".");
	} else {
		// cleans unneeded files
		auto cleanFiles = outputDir.entryInfoList(
			QStringList() << "evaluation-*.png" << "logger.csv",
			QDir::Files
		);
		for(auto cleanFilesIt = cleanFiles.begin(); cleanFilesIt != cleanFiles.end(); ++cleanFilesIt){
			QFile(cleanFilesIt->absoluteFilePath()).remove();
		}
	}
}


void ILP::logIterationHeader()
{
	cleanOutputDir();

	QFile file(outputDir.filePath(logFileName));
	bool success = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
	if(!success){
		throw std::logic_error(("Couldn't open log file: " + file.errorString()).toStdString().c_str());
	}
	QTextStream out(&file);
	
	out << "Iteration" << ';';

	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt)
		out << "Condition " << ((conditionsIt-conditions.cbegin()) + 1)  << ';';
	out << "Optimization Function" << '\n';
	file.close(); 
}


void ILP::logIterationResults(Evaluation *evaluation)
{
	QFile file(outputDir.filePath(logFileName));
	bool success = file.open(QIODevice::Append | QIODevice::Text);
	if(!success){
		throw std::logic_error(("Couldn't open log file: " + file.errorString()).toStdString().c_str());
	}
	QTextStream out(&file);
	
	out << currentIteration << ';';

	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt){
		out << (**conditionsIt) << ';';
	}
	out << (*evaluation) << '\n';
	file.close(); 
}



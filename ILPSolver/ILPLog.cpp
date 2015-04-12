#include "ILP.h"

#include <QTextStream>
#include "conditions/Condition.h"
#include "optimizations/OptimizationFunction.h"
#include "optimizations/Evaluation.h"
#include "conditions/ConditionPosition.h"
#include "conditions/HoleInSurfacePosition.h"
#include <qDebug>

const QString ILP::logFileName("solutions.csv");

void ILP::cleanOutputDir()
{
	if(!outputDir.exists()){
		outputDir.mkpath(".");
	} else {
		// cleans unneeded files
		auto cleanFiles = outputDir.entryInfoList(
			QStringList() << "evaluation-*.png" << logFileName,
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

	for(auto conditionsIt = conditions.cbegin(); conditionsIt != conditions.cend(); ++conditionsIt){
		auto header =  (*conditionsIt)->header();
		for(auto headerIt = header.begin(); headerIt != header.end(); ++headerIt){
			out << *headerIt << ";";
		}
	}

	auto header = optimizationFunction->header();
	for(auto headerIt = header.begin(); headerIt != header.end(); ++headerIt){
		out << *headerIt;
		out << ((headerIt != header.end()-1) ? ';': '\n');
	}
	file.close(); 
}


void ILP::logIterationResults(QVector<ConditionPosition *> positions, Evaluation *evaluation)
{
	QFile file(outputDir.filePath(logFileName));
	bool success = file.open(QIODevice::Append | QIODevice::Text);
	if(!success){
		throw std::logic_error(("Couldn't open log file: " + file.errorString()).toStdString().c_str());
	}
	QTextStream out(&file);
	
	out << currentIteration << ';';

	qDebug("Iteration: %d", currentIteration);

	for(auto positionsIt = positions.cbegin(); positionsIt != positions.cend(); ++positionsIt){
		auto info = (*positionsIt)->info();
		qDebug() <<  info;
		auto col3 =  dynamic_cast<HoleInSurfacePosition *>(*positionsIt)->transformation().getCol(3);
		qDebug("Transformation: (%0.2f, %0.2f, %0.2f)", col3.x, col3.y, col3.z);
		for(auto infoIt = info.begin(); infoIt != info.end(); ++infoIt){
			out << *infoIt  << ';';
		}
	}
	out << evaluation->info() << '\n';
	qDebug("\tResult: %s", qPrintable(evaluation->infoShort()));
	file.close(); 
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
		optimizationFunction->saveImage(getImageFileNameSolution(1 + it - bestConfigurations.begin()));
		QString evalInfo = it->evaluation()->info();
		QString evalInfoNow = evalSolutionNow->infoShort();
		logger->log(
			"\tEval: %s (now evals as %s)\n",
			evalInfo.toStdString().c_str(),
			evalInfoNow.toStdString().c_str()
		);
	}
}

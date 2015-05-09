#include "ILP.h"

#include <QTextStream>
#include "conditions/Condition.h"
#include "optimizations/SurfaceRadiosity.h"
#include "optimizations/SurfaceRadiosityEvaluation.h"
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
	
	out << "Iteration" << ";";

	for(auto condition: conditions){
		for(auto h: condition->header()){
			out << h << ";";
		}
	}

	for(auto h: optimizationFunction->header())
	{
		out << h << ";";
	}

	out << "Comment" << ";" 
		<< "Duration" << "\n";

	file.close(); 
}


void ILP::logIterationResults(
	QVector<ConditionPosition *> positions,
	SurfaceRadiosityEvaluation *evaluation,
	const QString& iterationComment,
	float evaluationDuration)
{
	QLocale locale;

	QFile file(outputDir.filePath(logFileName));
	bool success = file.open(QIODevice::Append | QIODevice::Text);
	if(!success){
		throw std::logic_error(("Couldn't open log file: " + file.errorString()).toStdString().c_str());
	}
	QTextStream out(&file);
	
	out << currentIteration << ';';

	for(auto position: positions){
		for(auto i: position->info()){
			out << i << ';';
		}
	}
	out << evaluation->info() << ';';
	out << locale.toString(evaluationDuration, 'f', 6) << ';';
	out << iterationComment << '\n';

	file.close(); 
}


void ILP::logBestConfigurations()
{
	logger->log("Best values(%d)\n", isoc.length());
	int i = 1;
	for(auto configuration: isoc){
		for(auto position: configuration.positions()){
			position->apply(renderer);
		}
		auto evalSolutionNow = optimizationFunction->evaluateRadiosity();
		optimizationFunction->saveImage(getImageFileNameSolution(i));
		QString evalInfo = configuration.evaluation()->info();
		QString evalInfoNow = evalSolutionNow->infoShort();
		logger->log(
			"\tEval %d: %s (now evals as %s)\n",
			i,
			evalInfo.toStdString().c_str(),
			evalInfoNow.toStdString().c_str()
		);
		++i;
	}
}

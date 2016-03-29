#include "Problem.h"

#include <QTextStream>
#include "conditions/Condition.h"
#include "optimizations/SurfaceRadiosity.h"
#include "optimizations/SurfaceRadiosityEvaluation.h"
#include "conditions/ConditionPosition.h"
#include <qDebug>

const QString Problem::logFileName("solutions.csv");

void Problem::cleanOutputDir()
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


void Problem::logIterationHeader()
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

	out << "Duration" << ";" 
		<< "Comment" << "\n";

	file.close(); 
}


void Problem::logIterationResults(
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


void Problem::logBestConfigurations()
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

static std::string toString(double d)
{
	QLocale locale;
	return locale.toString(d, 'f', 10).toStdString();
}


void Problem::logStatistics()
{
	logger->log("Statistics:\n");
	logger->log("Evaluations\t%d\n", statistics.evaluations);
	logger->log("Total time\t%s\n", toString(statistics.totalTime).c_str());

	auto timePerIteration = statistics.totalTime / statistics.evaluations;
	logger->log("Time per iteration\t%s\n", toString(timePerIteration).c_str());

	logger->log("Evaluation time\t%s\n", toString(statistics.evaluationTime).c_str());
	
	auto rendererStatistics = renderer->getStatistics();
	logger->log("Recalculate Acceleration Structures\t%s\n", toString(rendererStatistics.recalcAccelerationStructures).c_str());
	logger->log("Photon Tracing\t%s\n", toString(rendererStatistics.photonTracingTime).c_str());
	logger->log("Build Photon Map\t%s\n", toString(rendererStatistics.buildPhotonMapTime).c_str());
	logger->log("Resize Buffers\t%s\n", toString(rendererStatistics.resizeBufferTime).c_str());
	logger->log("Calculate Hit Count\t%s\n", toString(rendererStatistics.hitCountCalculationTime).c_str());

	auto otherTime = statistics.totalTime - (
		  rendererStatistics.photonTracingTime
		+ rendererStatistics.buildPhotonMapTime
		+ rendererStatistics.resizeBufferTime
		+ rendererStatistics.hitCountCalculationTime
		+ rendererStatistics.recalcAccelerationStructures);

	logger->log("Other\t%s\n", toString(otherTime).c_str());
}

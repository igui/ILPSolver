#include "ILP.h"

#include <QTextStream>
#include "conditions/Condition.h"
#include "optimizations/OptimizationFunction.h"
#include "optimizations/Evaluation.h"

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
		auto info = (*conditionsIt)->info();
		for(auto infoIt = info.begin(); infoIt != info.end(); ++infoIt){
			out << *infoIt  << ';';
		}
	}
	out << (*evaluation) << '\n';
	file.close(); 
}



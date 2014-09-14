/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <iostream>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <QCoreApplication>
#include <QCommandLineParser>
#include "ComputeDeviceRepository.h"
#include "renderer/PMOptixRenderer.h"
#include "logging/DummyLogger.h"
#include "scene/Scene.h"
#include "ILP.h"

void listDevices()
{
	ComputeDeviceRepository repository;
	const std::vector<ComputeDevice> & repo = repository.getComputeDevices();
	std::cout << "Available compute devices:" << std::endl;
	for(int i = 0; i < repo.size(); i++)
	{
		const ComputeDevice & device = repo.at(i);
		std::cout << "   " << i << ": " << device.getName()
			<< " (CC " <<  device.getComputeCapability() << ", "
			<< "Mem " << (int)qRound(device.getGlobalMemoryKB() / 1024.f) << "Mib, "
			<< "Clock " << (int)qRound(device.getClockFrequencyKHz() / 1000.0f) << "Mhz)" << std::endl;
	}
}


unsigned int calculateOptimalNumEmmitedPhotons(Logger& logger, PMOptixRenderer *renderer)
{
	static const unsigned int photonMapWidth = 2048;
	static const unsigned int photonMapSize = photonMapWidth * photonMapWidth;

	clock_t start = clock();
	renderer->genPhotonMap(photonMapWidth);
	auto hitCount = renderer->getHitCount();
	auto optimalN = hitCount;
	for(auto it = optimalN.begin(); it != optimalN.end(); ++it)
	{
		static const float k = 6146.566f; // 4*(z^2)/(0.05^2) where z is 1.96

		if(*it == 0)
		{
			*it = 0;
		}
		else
		{
			float p = *it / (float)photonMapSize;
			*it = ceilf(k * (1.0f - p) / p);
		}
	}
	

	double time = (clock() - start) / (double)CLOCKS_PER_SEC;
	logger.log("Photon map %d took %0.2fs\n", photonMapSize, time);
	
	logger.log("Id\tName\tHits\tHits/N\tOptimal N\tsqrt(Optimal N)\n"); 
	for(unsigned int i = 0; i < hitCount.size(); ++i)
	{
		logger.log("%3d %25s %7d %10f %10d %7d\n",
			i,
			renderer->idToObjectName(i).c_str(),
			hitCount.at(i), hitCount.at(i) / (float)photonMapSize,
			optimalN.at(i),
			(int)ceilf(sqrtf(optimalN.at(i)))); 
	}
	logger.log("\n");

	return 2048;
}


int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ilpsolver");
    QCoreApplication::setApplicationVersion("0.0.1");

	QCommandLineParser parser;
    parser.setApplicationDescription("ILP Problem solver using OptiX.");
    parser.addHelpOption();
    parser.addVersionOption();
	parser.addPositionalArgument("source", "Problem definition input file.");
	QCommandLineOption deviceOption(QStringList() << "d" << "device", "Device number id. Use -l to list devices.", "device", "0");
	QCommandLineOption listOption(QStringList() << "l" << "list", "List present CUDA devices in the machine.");
	parser.addOption(deviceOption);
	parser.addOption(listOption);

	parser.process(app);
	const QStringList args = parser.positionalArguments();
	if(args.isEmpty())
	{
		std::cerr << "Missing source parameter" << std::endl;
		parser.showHelp(1);
	}

	QString inputPath = args.first();

	if(parser.isSet(listOption))  // -l, then list available devices.
	{
		listDevices();
		exit(0);
	}
	
	// parse -d option
	bool parseOk;
	int deviceNumber = parser.value(deviceOption).toInt(&parseOk);
	if(!parseOk)
	{
		std::cerr << "Expect a number for device option." << std::endl;
		parser.showHelp(1);
	}
	if(deviceNumber < 0)
	{
		std::cerr << "Option --device(-d) can't be negative." << std::endl;
		parser.showHelp(1);
	}
	// find and check selected device
	ComputeDeviceRepository repository;
	const std::vector<ComputeDevice> & repo = repository.getComputeDevices();
	if(repo.empty())
	{
		std::cerr << "No computing device found"
			"You must have a CUDA enabled GPU to run this application. "
			"The CUDA GPUs nVidia developer page(https://developer.nvidia.com/cuda-gpus) has a "
			"list of all supported devices." << std::endl;
		exit(1);
	}
	if(deviceNumber >= repo.size())
	{
		std::cerr << "Invalid device number " << deviceNumber << "." << std::endl
			<<  "Try -l to list available computing devices." << std::endl;
		exit(1);
	}
	ComputeDevice device = repo.at(deviceNumber);

	// init device
	PMOptixRenderer renderer;
	DummyLogger logger;
	logger.log("Init device\n");
	renderer.initialize(device, &logger);
	logger.log("Load definition XML\n");
	logger.log("Load scene\n");
	ILP::fromFile(&logger, inputPath);
	logger.log("Renderer init scene\n");
	logger.log("Calculating optimal photons emmited per iteration\n");
	logger.log("Done! Cleaning up\n");
	
	return 0;
}


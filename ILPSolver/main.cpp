/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <iostream>
#include <cstdio>
#include <QCoreApplication>
#include <QCommandLineParser>
#include "ComputeDeviceRepository.h"
#include "renderer/PMOptixRenderer.h"
#include "logging/DummyLogger.h"
#include "scene/Scene.h"
#include "MinMaxAvg.h"

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

void calculateOptimalNumEmmitedPhotons(Logger& logger, PMOptixRenderer *renderer)
{
	static const unsigned int photonWidthSizes[] = { 32, 64, 128, 256, 512, 1024, 2048 };
	static const unsigned int numPhotonWidthSizes = sizeof(photonWidthSizes) / sizeof(photonWidthSizes[0]);
	static const unsigned int nPasses = 30;
	static const float acceptableAvgError = 0.05f;

	const int nMeshes = renderer->getHitCount().size();

	for(int i = 0; i < nMeshes; ++i)
	{
		logger.log("%d -> %s ", i, renderer->idToObjectName(i).c_str()); 
	}
	logger.log("\n");

	int optimalPhotonSize = photonWidthSizes[numPhotonWidthSizes-1];

	for(unsigned int i = 0; i < numPhotonWidthSizes; ++i)
	{
		std::vector<std::vector<unsigned int>> hitCounts(nPasses, std::vector<unsigned int>(nMeshes, 0));
		std::vector<float> averages(nMeshes, 0);

		float totalHits = 0.0f;

		// gather hit count and calculate average hits per mesh
		for(unsigned int pass = 0; pass < nPasses; ++pass)
		{
			renderer->genPhotonMap(photonWidthSizes[i]);
			hitCounts[pass] = renderer->getHitCount();
			for(int mesh = 0; mesh < nMeshes; ++mesh)
			{
				averages.at(mesh) += hitCounts.at(pass).at(mesh) / (float)nPasses;
			}
		}

		// calculate average absolute error (average - observed hit count)
		std::vector<float> averageError(nMeshes, 0);
		for(unsigned int pass = 0; pass < nPasses; ++pass)
		{
			for(int mesh = 0; mesh < nMeshes; ++mesh)
			{
				if(averages.at(mesh) != 0.0f)
					averageError.at(mesh) += fabsf(hitCounts.at(pass).at(mesh) - averages.at(mesh)) / (averages.at(mesh) * nPasses);
			}
		}

		if(averageError.empty())
		{
			break;
		}

		auto minMaxAvg = MinMaxAvg<float>::forVector(averageError);

		if(minMaxAvg.avg <= acceptableAvgError && photonWidthSizes[i] < optimalPhotonSize)
		{
			optimalPhotonSize = photonWidthSizes[i];
		}

		logger.log("Results for photonWidth = %d: min %0.1f%%, max %0.1f%%, avg %0.1f%%\n",
			photonWidthSizes[i], minMaxAvg.min * 100.0f, minMaxAvg.max * 100.0f, minMaxAvg.avg * 100.0f);
				
		logger.log("Average hit count(error%%): ");
		for(int mesh = 0; mesh < nMeshes; ++mesh)
		{
			logger.log("%0.0f(%0.1f%%) ", averages.at(mesh), averageError.at(mesh) * 100.f);
		}
		logger.log("\n");
	}

	logger.log("Optimal photon width size for average error of %0.0f%%: %d\n", acceptableAvgError * 100.0f, optimalPhotonSize);
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
	logger.log("Load scene\n");
	Scene *scene = Scene::createFromFile(&logger, "C:\\Users\\Igui\\Desktop\\cube.dae");
	logger.log("Renderer init scene\n");
	renderer.initScene(*scene);
	logger.log("Calculating optimal photons emmited per iteration\n");
	calculateOptimalNumEmmitedPhotons(logger, &renderer);
	logger.log("Done! Cleaning up\n");

	return 0;
}


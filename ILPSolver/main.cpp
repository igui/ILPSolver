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
	logger.log("Load definition XML & Scene\n");
	ILP ilp = ILP::fromFile(&logger, inputPath, &renderer);
	logger.log("Optimizing\n");
	ilp.optimize();
	logger.log("Done! Cleaning up\n");
	return 0;
}


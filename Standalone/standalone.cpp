/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include <iostream>
#include <exception>
#include <QApplication>
#include "gui/MainWindowBase.hxx"
#include "StandaloneApplication.h"
#include "ComputeDeviceRepository.h"
#include <QThread>
#include <QTextStream>
#include <QMessageBox>

int main( int argc, char** argv );

int CALLBACK WinMain(
  _In_  HINSTANCE hInstance,
  _In_  HINSTANCE hPrevInstance,
  _In_  LPSTR lpCmdLine,
  _In_  int nCmdShow
){
	return main(0, NULL);
}

int main( int argc, char** argv )
{
    QApplication qApplication(argc, argv);
    qApplication.setOrganizationName("Opposite Renderer");
    qApplication.setApplicationName("Opposite Renderer");

    try
    {
        ComputeDeviceRepository repository;

        const std::vector<ComputeDevice> & repo = repository.getComputeDevices();

		if(repo.empty())
		{
			QMessageBox::critical(NULL, "No computing device found", 
				"You must have a CUDA enabled GPU to run this application. "
				"The <a href='https://developer.nvidia.com/cuda-gpus'>CUDA GPUs</a> nVidia developer page has the "
				"a list of all supported devices. Application will now quit.");
			return 1;
		}

		ComputeDevice device = repo.front();

        StandaloneApplication application = StandaloneApplication(qApplication, device);

        // Run application
        QThread* applicationThread = new QThread(&qApplication);
        application.moveToThread(applicationThread);
        applicationThread->start();

        MainWindowBase mainWindow(application);
        mainWindow.showMaximized();

		application.rendererManager().logger().setSignalReceiver((QObject *) mainWindow.consoleDock());


		application.rendererManager().logger().log("Available compute devices:\n");

        for(int i = 0; i < repo.size(); i++)
        {
            const ComputeDevice & device = repo.at(i);
			application.rendererManager().logger().log("   %d: %s (CC %s PCI Bus %d)\n", i, device.getName(), device.getComputeCapability(), device.getPCIBusId() );
        }

		application.rendererManager().logger().log("Selected device 0\n"); // for now the app will use the first device

		if(argc >= 2)
		{
			// load scene by command line
			application.getSceneManager().setScene(argv[1]);
		}

        int returnCode = qApplication.exec();
        application.wait();

        applicationThread->quit();
        applicationThread->wait();

        return returnCode;
    }
    catch(const std::exception & E)
    {
        QMessageBox::warning(NULL, "Exception Thrown During Launch of Standalone", E.what());
        return -1;
    }
}

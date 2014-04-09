/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

/*
 * This is the render server main entry. The render server accepts connections from clients
 * and will perform rendering per request
 */

#include <iostream>
#include <exception>
#include <QObject>
#include "gui/ServerWindow.hxx"
#include <QApplication>
#include <QMessageBox>
#include "server/RenderServer.hxx"
//#include <vld.h>

int main( int argc, char** argv )
{
    QApplication app(argc, argv);
    app.setOrganizationName("Opposite Renderer");
    app.setApplicationName("Opposite Renderer");

	try {
		RenderServer renderServer;
		ServerWindow serverWindow(NULL, renderServer);
		serverWindow.show();
		int appCode = app.exec();
		renderServer.wait();
		return appCode;
	} catch(std::exception ex){
		QString error = QString("An unexpected error ocurred during execution:\n\t%1\nApplication will now quit.").arg(ex.what());
		QMessageBox::critical(nullptr, "Critical", error);
		return 1;
	}
}

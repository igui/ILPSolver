/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Application.hxx"
#include "MainWindowBase.hxx"
#include "renderer/OptixRenderer.h"
#include <QString>
#include <QMessageBox>
#include <QStandardPaths>
#include "RenderWidget.hxx"
#include "gui/AboutWindow.hxx"
#include "gui/ComputeDeviceInformationWidget.hxx"
#include "gui/docks/RenderInformationDock.hxx"
#include "gui/docks/OutputDock.hxx"
#include "gui/docks/PPMDock.hxx"
#include "gui/docks/CameraDock.hxx"
#include "gui/docks/SceneDock.hxx"
#include "gui/docks/ConsoleDock.hxx"
#include "ComputeDeviceRepository.h"
#include "config.h"
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include "gui/ui/resources.h"

const int MainWindowBase::maxRecentFiles = 5;

MainWindowBase::MainWindowBase(Application& application)
    : m_application(application),
      m_camera(application.getCamera())
{
    setupUi(this);
    actionReload_scene->setVisible(false);
    //connect(&application.getOutputSettingsModel(), SIGNAL(resolutionUpdated()), this, SLOT(onOutputSettingsResolutionUpdated()));
    //connect(&application.getPPMSettingsModel(), SIGNAL(updated()), this, SLOT(onPPMSettingsModelUpdated()));

    connect(&application, SIGNAL(applicationError(QString)), this, SLOT(onApplicationError(QString)));

    // Render Information Dock
    RenderInformationDock* renderInformationDock = new RenderInformationDock(this, application.getRenderStatisticsModel(), application);
    this->addDockWidget(Qt::RightDockWidgetArea, renderInformationDock);
    connect(renderInformationDock, SIGNAL(renderStatusToggle()), this, SLOT(onRenderStatusToggle()));
    connect(renderInformationDock, SIGNAL(renderRestart()), this, SLOT(onRenderRestart()));

    // Output Dock

    OutputDock* outputDock = new OutputDock(this, application.getOutputSettingsModel());
    this->addDockWidget(Qt::RightDockWidgetArea, outputDock);

    // PPM Information dock

    PPMDock* ppmDock = new PPMDock(this, m_application, m_application.getPPMSettingsModel());
    this->addDockWidget(Qt::RightDockWidgetArea, ppmDock);

    // Camera Dock

    CameraDock* cameraDock = new CameraDock(this, application.getCamera(), application.getPPMSettingsModel(), application.getOutputSettingsModel());
    this->addDockWidget(Qt::RightDockWidgetArea, cameraDock);
    connect(&application, SIGNAL(cameraUpdated()), cameraDock, SLOT(onCameraUpdated()));
    connect(cameraDock, SIGNAL(cameraUpdated()), &application, SLOT(onCameraUpdated()));

    // Scene Dock

    SceneDock* sceneDock = new SceneDock(this, application.getSceneManager());
    this->addDockWidget(Qt::RightDockWidgetArea, sceneDock);

	// Console Dock
	ConsoleDock *consoleDock = new ConsoleDock(this);
	this->m_consoleDock = consoleDock;
	this->addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Status Bar Running Status and Time

    m_statusbar_runningStatusLabel = new QLabel(this);
    this->statusBar()->addPermanentWidget(m_statusbar_runningStatusLabel, 1);
    m_statusbar_runningStatusLabel->setGeometry(100, 0, 100, 12);
    QTimer* runningStatusLabelTimer = new QTimer(this);
    runningStatusLabelTimer->setInterval(100);
    connect(runningStatusLabelTimer, SIGNAL(timeout()), this, SLOT(onUpdateRunningStatusLabelTimer()));
    runningStatusLabelTimer->start();

    // Status Bar Render Method

    m_statusbar_renderMethodLabel = new QLabel(this);
    this->statusBar()->addPermanentWidget(m_statusbar_renderMethodLabel, 1);
    m_statusbar_renderMethodLabel->setGeometry(100, 0, 100, 12);

    // Render Widget

    m_renderWidget = new RenderWidget(centralwidget, application.getCamera(), application.getOutputSettingsModel());
    gridLayout->addWidget(m_renderWidget, 0, 0, 1, 1);
    connect(m_renderWidget, SIGNAL(cameraUpdated()), &application, SLOT(onCameraUpdated()));
    connect(&application, SIGNAL(newFrameReadyForDisplay(const float*, unsigned long long)), 
            m_renderWidget, SLOT(onNewFrameReadyForDisplay(const float*, unsigned long long)),
            Qt::QueuedConnection);

    connect(&application, SIGNAL(runningStatusChanged()), this, SLOT(onRunningStatusChanged()));
    connect(&application, SIGNAL(renderMethodChanged()), this, SLOT(onRenderMethodChanged()));
    connect(this, SIGNAL(renderRestart()), &m_application, SLOT(onRenderRestart()));
    connect(this, SIGNAL(renderStatusToggle()), &m_application, SLOT(onRenderStatusToggle()));


	// recent files
	for (int i = 0; i < maxRecentFiles; ++i) {
		QAction *action = new QAction(this);
		action->setVisible(false);
		connect(action, SIGNAL(triggered()),
				this, SLOT(onOpenRecentFile()));
		menuFile->insertAction(actionQuit, action);
		m_recentFileActions.append(action);
	}
	m_recentActionsSeparator = new QAction(this);
	m_recentActionsSeparator->setSeparator(true);
	menuFile->insertAction(actionQuit, m_recentActionsSeparator);

	connect(this, SIGNAL(recentFilesChanged()), this, SLOT(onRecentFilesChanged()));
	connect(&application.getSceneManager(), SIGNAL(sceneUpdated()), this, SLOT(onSceneUpdated()));

    onRunningStatusChanged();
    onRenderMethodChanged();
	onRecentFilesChanged();
}

void MainWindowBase::closeEvent( QCloseEvent* event )
{
    
}

ConsoleDock *MainWindowBase::consoleDock()
{
	return m_consoleDock;
}

MainWindowBase::~MainWindowBase()
{

}

void MainWindowBase::onActionAbout()
{
    AboutWindow* aboutWindow = new AboutWindow(this);
    aboutWindow->show();
    connect(aboutWindow, SIGNAL(finished()), aboutWindow, SLOT(deleteLater()));
}

void MainWindowBase::onRenderStatusToggle()
{
    emit renderStatusToggle();
}

void MainWindowBase::onSetCameraToDefault()
{
    m_application.setCameraToSceneDefault();
}

void MainWindowBase::onChangeRenderMethodPM()
{
	m_application.setRenderMethod(RenderMethod::PHOTON_MAPPING);
    emit renderRestart();
}

void MainWindowBase::onChangeRenderMethodPPM()
{
    m_application.setRenderMethod(RenderMethod::PROGRESSIVE_PHOTON_MAPPING);
    emit renderRestart();
}

void MainWindowBase::onChangeRenderMethodPT()
{
    m_application.setRenderMethod(RenderMethod::PATH_TRACING);
    emit renderRestart();
}

void MainWindowBase::onConfigureGPUDevices()
{
    /*QDialog* dialog = new QDialog(this);
    ComputeDeviceInformationWidget* widget = new ComputeDeviceInformationWidget(dialog, ComputeDeviceRepository::get());
    dialog->show();*/
}

void MainWindowBase::onOpenSceneFile()
{
	QString desktopDir = QStandardPaths::locate(QStandardPaths::DesktopLocation, "", QStandardPaths::LocateDirectory);

    QString fileName = QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			desktopDir,
			tr("Scene files (*.dae *.blend *.3ds);;Any(*.*)")
	);
    if(fileName.length() > 0)
    {
        m_lastOpenedSceneFile = QFileInfo(fileName);
        actionReload_scene->setText(QString("Reload last scene (%1)").arg(m_lastOpenedSceneFile.completeBaseName()));
        actionReload_scene->setVisible(true);
        loadSceneByName(fileName);
    }
}

void MainWindowBase::onRunningStatusChanged()
{
    if(m_application.getRunningStatus() == RunningStatus::RUNNING)
    {
        actionRenderStatusToggle->setText("Pause");
    }
    else if(m_application.getRunningStatus() == RunningStatus::STOPPED)
    {
        actionRenderStatusToggle->setText("Start");
    }
}

void MainWindowBase::onRenderMethodChanged()
{
    QString str;
    if(m_application.getRenderMethod() == RenderMethod::PATH_TRACING)
    {
        str = "Path Tracing";
    }
    else
    {
        str = "Progressive Photon Mapping";
        if(ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_UNIFORM_GRID)
        {
            str += " (Sorted uniform grid)";
        }
        else if(ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_KD_TREE_CPU)
        {
            str += " (CPU k-d tree)";
        }
        else if(ACCELERATION_STRUCTURE == ACCELERATION_STRUCTURE_STOCHASTIC_HASH)
        {
            str += " (Stochastic hash)";
        }
    }

    m_statusbar_renderMethodLabel->setText(QString("Render method: ") + str);
}

void MainWindowBase::onUpdateRunningStatusLabelTimer()
{
    m_statusbar_runningStatusLabel->setText(QString("Status: ") + getApplicationStatusString(m_application));
}

void MainWindowBase::loadSceneByName( QString & sceneName )
{
    try
    {
        m_application.getSceneManager().setScene(sceneName.toUtf8().constData());
    }
    catch(const std::exception & E)
    {
        QMessageBox::warning(this, "Error loading scene file", QString(E.what()));
    }
    catch(...)
    {
        QMessageBox::warning(this, "Unknown error", "Unknown error happened when importing this scene. This is not a valid scene.");
    }
}

void MainWindowBase::onReloadLastScene()
{
    if(m_lastOpenedSceneFile.exists())
    {
        loadSceneByName(m_lastOpenedSceneFile.canonicalFilePath());
    }
}

void MainWindowBase::onActionOpenBuiltInScene()
{
    bool ok;
    QString sceneName = QInputDialog::getText(this, tr("Please provide a built-in scene name"),
                                           tr("Name (customscene/ class name):"), QLineEdit::Normal, "", &ok);

    if (ok && !sceneName.isEmpty())
    {
        loadSceneByName(sceneName);
    }
}

void MainWindowBase::onRenderRestart()
{
    emit renderRestart();
}

void MainWindowBase::onApplicationError( QString appError )
{
    QMessageBox::warning(this, "An application error occurred", appError);
}

QString MainWindowBase::getApplicationStatusString(const Application & application, bool showSeconds)
{
    QString status = "";

    if(application.getSceneManager().getStatus() == SceneManagerStatus::IMPORTING)
    {
        status += "Importing scene";
    }
    else if(application.getRunningStatus() == RunningStatus::STOPPED)
    {
        status += "Stopped";
    }
    else if(application.getRunningStatus() == RunningStatus::PAUSE)
    {
        status += "Pause";
    }
    else
    {
        if(application.getRendererStatus() == RendererStatus::NOT_INITIALIZED)
        {
            status += "Not initialized";
        }
        else if(application.getRendererStatus() == RendererStatus::INITIALIZING_ENGINE)
        {
            status += "Initializing engine";
        }
        else if(application.getRendererStatus() == RendererStatus::INITIALIZING_SCENE)
        {
            status += "Initializing scene";
        }
        else  if(application.getRendererStatus() == RendererStatus::RENDERING)
        {
            status += "Running";
            if(showSeconds)
            {
                status += QString(" (%1 seconds)").arg(application.getRenderTimeSeconds(), 0, 'f', 1);
            }
        }
    }
    return status;
}

void MainWindowBase::onActionSaveImagePPM()
{
    printf("Save image as PPM!");
}

void MainWindowBase::onRecentFilesChanged()
{	
     QStringList files = QSettings().value("recentFileList").toStringList();

     int numRecentFiles = qMin(files.size(), maxRecentFiles);

     for (int i = 0; i < numRecentFiles; ++i) {
		 QString strippedName = QFileInfo(files[i]).fileName();

         QString text = tr("&%1 %2").arg(i + 1).arg(strippedName);
		 m_recentFileActions[i]->setText(text);
         m_recentFileActions[i]->setData(files[i]);
         m_recentFileActions[i]->setVisible(true);
     }
     for (int j = numRecentFiles; j < maxRecentFiles; ++j)
         m_recentFileActions[j]->setVisible(false);

	 m_recentActionsSeparator->setVisible(numRecentFiles > 0);
}

void MainWindowBase::onOpenRecentFile()
 {
     QAction *action = qobject_cast<QAction *>(sender());
     if (action){
         QString fileName = action->data().toString();
		 loadSceneByName(fileName);
	 }
 }

void MainWindowBase::onSceneUpdated()
{
	QString fileName = m_application.getSceneManager().getScene()->getSceneName();
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > maxRecentFiles)
		files.removeLast();
	settings.setValue("recentFileList", files);
	emit onRecentFilesChanged();
}
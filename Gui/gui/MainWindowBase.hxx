/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once
#include <QtGui>
#include <QMainWindow>
#include "ui/ui_MainWindowBase.h"
#include "gui_export_api.h"
#include <QString>
#include <qlabel.h>

class RenderWidget;
class OptixRenderer;
class Application;
class Camera;
class ConsoleDock;

class MainWindowBase : public QMainWindow, public Ui::MainWindowBase
{
    Q_OBJECT
public:
    GUI_EXPORT_API MainWindowBase(Application& application);
    GUI_EXPORT_API ~MainWindowBase();
    GUI_EXPORT_API virtual void closeEvent(QCloseEvent* event);
    static QString getApplicationStatusString(const Application & application, bool showSeconds = true);

	GUI_EXPORT_API ConsoleDock *consoleDock();

signals:
    void renderRestart();
    void renderStatusToggle();
	void recentFilesChanged();

private slots:
    GUI_EXPORT_API_QT void onSetCameraToDefault();
    GUI_EXPORT_API_QT void onChangeRenderMethodPPM();
    GUI_EXPORT_API_QT void onChangeRenderMethodPT();
    GUI_EXPORT_API_QT void onConfigureGPUDevices();
    void onOpenSceneFile();
    void onReloadLastScene();
    void onRunningStatusChanged();
    void onRenderMethodChanged(); 
    void onActionAbout();
    void onRenderStatusToggle();
    void onRenderRestart();
    void onUpdateRunningStatusLabelTimer();
    void onApplicationError(QString);
    void onActionOpenBuiltInScene();
    void onActionSaveImagePPM();
	void onRecentFilesChanged();
	void onOpenRecentFile();
	void onSceneUpdated();

private:
	// max recent files in the File menu
	static const int maxRecentFiles;

    void loadSceneByName( QString &fileName );
    RenderWidget* m_renderWidget;
    ConsoleDock* m_consoleDock;
    Application & m_application;
    QLabel* m_statusbar_renderMethodLabel;
    QLabel* m_statusbar_runningStatusLabel;
    unsigned int m_renderWidth;
    unsigned int m_renderHeight;
    QFileInfo m_lastOpenedSceneFile;
    Camera & m_camera;

	QAction *m_recentActionsSeparator;
	QList<QAction *> m_recentFileActions;
};

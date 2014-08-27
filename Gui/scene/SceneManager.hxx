/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include "scene/Scene.h"
#include <QObject>
#include "gui_export_api.h"
#include "logging/Logger.h"

namespace SceneManagerStatus
{
    enum E {NO_SCENE, IMPORTING, HAS_SCENE};

}

class SceneManager : public QObject
{
    Q_OBJECT;
public:
    SceneManager(void);
    ~SceneManager(void);
    GUI_EXPORT_API_QT Scene* getScene() const;
    GUI_EXPORT_API_QT void setScene(const char* sceneName);
	GUI_EXPORT_API_QT void setLogger(Logger *logger);
    SceneManagerStatus::E getStatus() const;
signals:
    void sceneLoadingNew();
    void sceneUpdated();
    void sceneLoadError(QString error);
private slots:
    void onLoadNewScene(QString sceneName);
private:
    SceneManagerStatus::E m_status;
    Scene* m_scene;
	Logger *m_logger;
};
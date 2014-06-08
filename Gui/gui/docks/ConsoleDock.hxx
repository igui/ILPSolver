#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QDockWidget>
#include "ui/ui_ConsoleDock.h"

namespace Ui {
class ConsoleWidget;
}

class ConsoleDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ConsoleDock(QWidget *parent = 0);
    ~ConsoleDock();

private:
    Ui::ConsoleDock *ui;
};

#endif // CONSOLEWIDGET_H

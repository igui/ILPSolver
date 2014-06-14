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
	static const int maximumLines;
    explicit ConsoleDock(QWidget *parent = 0);
    ~ConsoleDock();
public slots:
	void log(const QString&, const QString&);

private:
    Ui::ConsoleDock *ui;
};

#endif // CONSOLEWIDGET_H

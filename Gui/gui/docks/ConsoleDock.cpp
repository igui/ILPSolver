#include "ConsoleDock.hxx"

ConsoleDock::ConsoleDock(QWidget *parent) :
	QDockWidget(parent),
    ui(new Ui::ConsoleDock)
{
    ui->setupUi(this);
}

ConsoleDock::~ConsoleDock()
{
    delete ui;
}

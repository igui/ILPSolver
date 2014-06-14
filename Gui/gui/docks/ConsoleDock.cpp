#include "ConsoleDock.hxx"

const int ConsoleDock::maximumLines = 10000;

ConsoleDock::ConsoleDock(QWidget *parent) :
	QDockWidget(parent),
    ui(new Ui::ConsoleDock)
{
    ui->setupUi(this);
	ui->console->document()->setMaximumBlockCount(maximumLines);
}

void ConsoleDock::log(const QString& style, const QString& message)
{
	QTextEdit& textEdit = *(ui->console);

	textEdit.moveCursor (QTextCursor::End);
	textEdit.insertPlainText (message);
	textEdit.moveCursor (QTextCursor::End);
}

ConsoleDock::~ConsoleDock()
{
    delete ui;
}

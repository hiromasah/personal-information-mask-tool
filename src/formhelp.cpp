#include "formhelp.h"
#include "ui_formhelp.h"
#include <QtGui>

FormHelp::FormHelp(QWidget *parent, QString programName) :
    QWidget(parent),
    m_ui(new Ui::FormHelp)
{
    m_ui->setupUi(this);

    setWindowTitle(programName + " " + tr("help"));

    QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    QTextDecoder *decoder = codec->makeDecoder();

    QFile file("help.html");
    if (file.open(QIODevice::ReadOnly)){
        m_ui->textBrowser->append(decoder->toUnicode(file.readAll()));
        m_ui->textBrowser->moveCursor(QTextCursor::Start);
    }
}

FormHelp::~FormHelp()
{
    delete m_ui;
}

void FormHelp::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

#ifndef FORMHELP_H
#define FORMHELP_H

#include <QtGui/QWidget>

namespace Ui {
    class FormHelp;
}

class FormHelp : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(FormHelp)
public:
    explicit FormHelp(QWidget *parent = 0, QString programName = "");
    virtual ~FormHelp();
protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::FormHelp *m_ui;
};

#endif // FORMHELP_H

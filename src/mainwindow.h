#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include "formhelp.h"
#include "processthread.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString processExecutionTimeoutSeconds;
    QString formalProgramName, programName, programVersion;
    QString meCabHome_x86, meCabHome, maskColumnNo;
    int doneFileCount;

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    FormHelp *formHelp;
    PThread *pthread;

    bool CheckMeCabHome(QSettings *ini_settings);
    void getTargetFiles(QListWidget *listWidget, QLineEdit *labelFileStatus, QStringList files);
    bool getTargetFolder(QListWidget *listWidget, QString folder);
    bool getTargetFile(QListWidget *listWidget, QString fpath, int cnt);
    void rewriteFileStatus(QListWidget *listWidget, QLineEdit *labelFileStatus);
    int isDup(QListWidget *listWidget, QString fpath);
    void eraseFilesInFolder(QString folder);
    bool checkFileTailInFolder(QString folder, QString tail);
    bool makeMeCabInputFile(QString orgFilePath, QString meCabInputPath, QString maskColumnNo);
    bool editOutputFile(QString inputPath, QString meCabOutputPath, QString outputPath, QString maskColumnNo);

    QString inputFilesPath;
    QString outputFilesPath;

private slots:
    void on_actionHelp_triggered();
    void on_action_About_triggered();
    void on_action_Exit_triggered();
    void on_pushButton_Exec_clicked();
    void on_pushButton_Exit_clicked();
    void on_pushButton_Ref_clicked();
    void on_pushButton_Delete_EF_clicked();
    void on_pushButton_Add_EF_clicked();
    void timerProc();
    void pthreadDone();
};

#endif // MAINWINDOW_H

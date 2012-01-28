#include <QtGui>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "formhelp.h"
#include "processthread.h"

//------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName(programName); //Avoid QSettings trouble in Vista x64
    QCoreApplication::addLibraryPath("plugins");

    ui->setupUi(this);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerProc()));

    programVersion = "0.2";
    programName = "PMP";
    formalProgramName = "Privacy Mask Program";

    setWindowTitle(tr(formalProgramName.toAscii()) + "(" + programName + ")");

    // Prepare help widget
    formHelp = new FormHelp(0, tr(formalProgramName.toAscii()) + "(" + programName + ")");

    pthread = new PThread();
    connect(pthread, SIGNAL(finished()), this, SLOT(pthreadDone()));

    QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    QTextDecoder *decoder = codec->makeDecoder();

    QString iniFname = programName + ".ini";
    QSettings *ini_settings;
    ini_settings = new QSettings(iniFname, QSettings::IniFormat);

    // Get MeCab_HOME
    meCabHome = decoder->toUnicode(ini_settings->value("MeCab/MeCab_HOME").toByteArray());
    if(meCabHome == ""){
        meCabHome = "c:/Program Files (x86)/MeCab/bin/";
    }

    // Get Mask Column No
    maskColumnNo = ini_settings->value("Target/MaskColumnNo").toString();
    if(maskColumnNo == ""){
        maskColumnNo = "";
    }

    // Get Process Execution Timeout (default 600sec)
    processExecutionTimeoutSeconds = ini_settings->value("Timeout/ProcessExecution").toString();
    if(processExecutionTimeoutSeconds == ""){
        processExecutionTimeoutSeconds = "600";
    }

    // Check the path to GnuPG
    if(! CheckMeCabHome(ini_settings)){
        timer->start(1000);
        return;
    }

    ui->listWidgetStatus->addItem(tr("Processing status."));
}

//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

//------------------------------------------------------------------------------
void MainWindow::timerProc()
{
    timer->stop();
    close();
}

//------------------------------------------------------------------------------
bool MainWindow::CheckMeCabHome(QSettings *ini_settings)
{
    bool stat = true;
    QString meCabStdDir, meCabStdDir_x86;

    QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    QTextEncoder *encoder = codec->makeEncoder();

    QDir dir(meCabHome);
    if(! dir.exists()){
        meCabStdDir_x86 = "c:/Program Files (x86)/MeCab/bin/";
        QDir dir_x86(meCabStdDir_x86);
        meCabStdDir = "c:/Program Files/MeCab/bin/";
        QDir dir(meCabStdDir);
        if(dir_x86.exists()){
            meCabHome = meCabStdDir_x86;
        }
        else if(dir.exists()){
            meCabHome = meCabStdDir;
        }
        else{
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(this, tr("ERROR"),
                tr("MeCab is not installed in the folder below.") + "\n" +
                " " + meCabStdDir + "\n" +
                " " + tr("nor") + "\n" +
                " " + meCabStdDir_x86 + "\n\n" +
                tr("Install MeCab.") + "\n" +
                tr("If MeCab is installed in another folder, Change ini file.") + "\n" +
                " " + tr("ini file name") + ": " + programName + ".ini\n" +
                " " + tr("section") + ": [MeCab]\n" +
                " " + tr("name") + ": MeCab_HOME\n" +
                " " + tr("ex.") + " MeCab_HOME=C:/Program Files (x86)/MeCab/bin/\n"
                ,QMessageBox::Abort);
            meCabHome = "";
        }
        if(meCabHome != ""){
            ini_settings->setValue("MeCab/MeCab_HOME", QString(encoder->fromUnicode(meCabHome)));
        }
        else{
            stat = false;
        }
    }
    return stat;
}

//------------------------------------------------------------------------------
void MainWindow::on_pushButton_Delete_EF_clicked()
{
    QStringList wlist;
    for (int ii = 0; ii < ui->listWidget_EF->count(); ii++) {
        if (! ui->listWidget_EF->item(ii)->isSelected()) {
            wlist.append(ui->listWidget_EF->item(ii)->text());
        }
    }
    ui->listWidget_EF->clear();
    ui->listWidget_EF->addItems(wlist);

    rewriteFileStatus(ui->listWidget_EF, ui->labelFileStatus_EF);
}

//------------------------------------------------------------------------------
void MainWindow::on_pushButton_Add_EF_clicked()
{
    QFileDialog::Options options;
//        options |= QFileDialog::DontUseNativeDialog;
    QString selectedFilter;
    QStringList files = QFileDialog::getOpenFileNames(
                                this, tr("Select the Input File(s)."),
                                inputFilesPath,
                                tr("All Files (*)"),
                                &selectedFilter,
                                options);
    if (files.count()) {
        getTargetFiles(ui->listWidget_EF, ui->labelFileStatus_EF, files);
    }
}

//------------------------------------------------------------------------------
void MainWindow::getTargetFiles(QListWidget *listWidget, QLineEdit *labelFileStatus, QStringList files)
{
    for (int ii = 0; ii < files.count(); ii++) {
        QFileInfo *fileinfo = new QFileInfo(files[ii]);
        if(fileinfo->isFile()){
            if(! getTargetFile(listWidget, files[ii], files.count() - ii - 1)){
                break;
            }
            inputFilesPath = fileinfo->dir().absolutePath();
        }
        else{
            QMessageBox::StandardButton reply;
            reply = QMessageBox::information(this, tr("Confirm"),
                     tr("Select all files in the folder?") + "\n\n" + files[ii],
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
            if (reply == QMessageBox::Yes){
                if(! getTargetFolder(listWidget, files[ii])){
                    break;
                }
                inputFilesPath = files[ii];
            }
            else if(reply == QMessageBox::No){
                continue;
            }
            else{
                break;
            }
        }
    }
    rewriteFileStatus(listWidget, labelFileStatus);
}

//------------------------------------------------------------------------------
bool MainWindow::getTargetFolder(QListWidget *listWidget, QString folder)
{
    bool isContinue = true;

    QDir dir(folder);
    QStringList files = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable);
    for( int ii = 0; ii < files.count(); ii++){
        QFileInfo *fileinfo = new QFileInfo(folder + "/" + files[ii]);
        if(fileinfo->isFile()){
            if(!getTargetFile(listWidget, folder + "/" + files[ii], files.count() - ii - 1)){
                isContinue = false;
                break;
            }
        }
        else{
            QMessageBox::StandardButton reply;
            reply = QMessageBox::information(this, tr("Confirm"),
                     tr("Select all files in the folder?") + "\n\n" + folder + "/" + files[ii],
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
            if (reply == QMessageBox::Yes){
                if(! getTargetFolder(listWidget, folder + "/" + files[ii])){
                    isContinue = false;
                    break;
                }
            }
            else if (reply == QMessageBox::No){
                continue;
            }
            else{
                isContinue = false;
                break;
            }
        }
    }

    return isContinue;
}

//------------------------------------------------------------------------------
bool MainWindow::getTargetFile(QListWidget *listWidget, QString fpath, int rest)
{
    bool isContinue = true;
    int dupIndex;

    dupIndex = isDup(listWidget, fpath);
    if (dupIndex != -1) {
        QMessageBox::StandardButton reply;
        if(rest > 0){
            reply = QMessageBox::information(this, tr("Confirm"),
                tr("Duplicate file name. Replace?") + "\n\n" + listWidget->item(dupIndex)->text() + "\n   " + tr("to") + "\n" + fpath,
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
            if(reply == QMessageBox::Abort){
                isContinue = false;
            }
            else if(reply == QMessageBox::Yes){
                listWidget->item(dupIndex)->setText(fpath);
            }
        }
        else{
            reply = QMessageBox::information(this, tr("Confirm"),
                tr("Duplicate file name. Replace?") + "\n\n" + listWidget->item(dupIndex)->text() + "\n   " + tr("to") + "\n" + fpath,
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){
                listWidget->item(dupIndex)->setText(fpath);
            }
        }
    }
    else{
        listWidget->addItem(fpath);
    }
    return isContinue;
}

//------------------------------------------------------------------------------
void MainWindow::rewriteFileStatus(QListWidget *listWidget, QLineEdit *labelFileStatus)
{
    int fileCount = 0;
    qint64 totalFileSize = 0;
    qint64 tera = Q_INT64_C(1099511627776);
    qint64 peta = Q_INT64_C(1125899906842624);

    for (int ii = 0; ii < listWidget->count(); ii++)
    {
        QString fpath = listWidget->item(ii)->text();
        QFile *file = new QFile(fpath);
        totalFileSize += file->size();
        fileCount++;
    }
    QString fileSizeString = "";
    if (totalFileSize == 0)
    {
        fileSizeString = "0";
    }
    else
    {
        fileSizeString = QString("%L2").arg(totalFileSize);
    }
    QString aboutFileSizeString = "";
    int aboutFileSize;
    if(totalFileSize < 1024){
        aboutFileSizeString = "";
    }
    else if(totalFileSize < 1024 * 1024){
        aboutFileSize = (int)(totalFileSize / 1024);
        aboutFileSizeString = " (" + tr("about") + QString("%1").arg(aboutFileSize) + tr("Kbyte") + ")";
    }
    else if (totalFileSize < 1073741824) // ‚PƒMƒK 1024 * 1024 * 1024
    {
        aboutFileSize = (int)(totalFileSize / (1024 * 1024));
        aboutFileSizeString = " (" + tr("about") + QString("%1").arg(aboutFileSize) + tr("Mbyte") + ")";
    }
    else if (totalFileSize < tera) // ‚Pƒeƒ‰ 1024 * 1024 * 1024 * 1024
    {
        aboutFileSize = (int)(totalFileSize / (1073741824));
        aboutFileSizeString = " (" + tr("about") + QString("%1").arg(aboutFileSize) + tr("Gbyte") + ")";
    }
    else if (totalFileSize < peta) // ‚Pƒyƒ^ 1024 * 1024 * 1024 * 1024 * 1024
    {
        aboutFileSize = (int)(totalFileSize / (tera));
        aboutFileSizeString = " (" + tr("about") + QString("%1").arg(aboutFileSize) + tr("Tbyte") + ")";
    }

    labelFileStatus->setText(tr("Selected File Count : ") + QString("%1").arg(fileCount) + "  " + tr("Total File Size : ") + fileSizeString + aboutFileSizeString);
}

//------------------------------------------------------------------------------
int MainWindow::isDup(QListWidget *listWidget, QString fpath)
{
    int index = -1;

    QFileInfo *fileinfo = new QFileInfo(fpath);
    QString fname = fileinfo->fileName();
    for (int ii = 0; ii < listWidget->count(); ii++) {
        if(listWidget->item(ii)->text() == fpath) {
            index = ii;
            break;
        }
        QFileInfo fi(listWidget->item(ii)->text());
        if(fname == fi.fileName()){
            index = ii;
            break;
        }
    }
    return index;
}

//------------------------------------------------------------------------------
void MainWindow::eraseFilesInFolder(QString folder)
{
    QDir dir(folder);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable);
    for( int ii = 0; ii < files.count(); ii++){
        QFile::remove(folder + "/" + files[ii]);
    }
}

//------------------------------------------------------------------------------
bool MainWindow::checkFileTailInFolder(QString folder, QString tail)
{
    bool stat = false;
    
    QDir dir(folder);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable);
    for( int ii = 0; ii < files.count(); ii++){
        if(files[ii].endsWith(tail)){
            stat = true;
            break;
        }
    }
    return stat;
}

//------------------------------------------------------------------------------
bool MainWindow::makeMeCabInputFile(QString orgFilePath, QString meCabInputPath, QString maskColumnNo)
{
    bool stat = true;
    
    QFile wfile(meCabInputPath);
    if (wfile.open(QIODevice::WriteOnly | QIODevice::Text)){
        QFile rfile(orgFilePath);
        if (rfile.open(QIODevice::ReadOnly | QIODevice::Text)){
            while (!rfile.atEnd()) {
                QByteArray line = rfile.readLine();
                if(maskColumnNo == ""){
                    wfile.write(line);
                }
                else{
                    QString str = line;
                    QStringList wlist = str.split("\t", QString::KeepEmptyParts, Qt::CaseSensitive);
                    bool ok;
                    int col = maskColumnNo.toInt(&ok, 10);
                    if(ok){
                        QString wstr = wlist[col];
                        wstr.remove(QRegExp("\r\n$"));
                        wfile.write((wstr + "\r\n").toAscii());
                    }
                    else{
                        QMessageBox::StandardButton reply;
                        reply = QMessageBox::critical(this, tr("ERROR"),
                            tr("Illegal Mask Column No.") + "(" + maskColumnNo + ")",
                            QMessageBox::Abort);
                        if (reply == QMessageBox::Abort){
                            wfile.close();
                            close();
                        }
                        stat = false;
                        break;
                    }
                }
            }
            rfile.close();
        }
        else{
            // Can't open input file.
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(this, tr("ERROR"),
                tr("Can't open the input file.") + "(" + orgFilePath + ")",
                QMessageBox::Abort);
            if (reply == QMessageBox::Abort){
                wfile.close();
                close();
            }
            stat = false;
        }
        
        wfile.close();
    }
    else{
        // Can't open the work file.
        QMessageBox::StandardButton reply;
        reply = QMessageBox::critical(this, tr("ERROR"),
            tr("Can't generate the work file.") + "(" + meCabInputPath + ")",
            QMessageBox::Abort);
        if (reply == QMessageBox::Abort){
            close();
        }
        stat = false;
    }
    
    return stat;
}

//------------------------------------------------------------------------------
void MainWindow::on_pushButton_Ref_clicked()
{
    QFileDialog::Options options;
    QString selectedFilter;
    options |= QFileDialog::ShowDirsOnly;
    QString folderName = QFileDialog::getExistingDirectory(this,
                                tr("Select the output folder."),
                                outputFilesPath,
                                options);
    if (!folderName.isEmpty()){
        ui->lineEdit_OutputFolder->setText(folderName);
        outputFilesPath = folderName;
    }
}

//------------------------------------------------------------------------------
void MainWindow::on_pushButton_Exit_clicked()
{
    close();
}

//------------------------------------------------------------------------------
void MainWindow::on_pushButton_Exec_clicked()
{
    bool stat = true;
    QString copyErrorPath;
    //QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    //QTextDecoder *decoder = codec->makeDecoder();
    QString errmsg = "";
    QString workingFolder = "tmp";
    QString meCabBatchPath = "tmp/meCab.bat";
    QString meCabInputPath = "tmp/meCabInput.txt";
    QString meCabOutputPath = "tmp/meCabOutput.txt";
    QMessageBox::StandardButton reply;

    // Check if the input files is selected.
    if(ui->listWidget_EF->count() == 0){
        errmsg += tr("Select the input file(s).") + "\n";
    }

    // Check if the output folder is selected.
    if(ui->lineEdit_OutputFolder->text() == ""){
        errmsg += tr("Select the output folder.") + "\n";
    }

    if(errmsg == ""){
        reply = QMessageBox::information(this, tr("Confirm"),
            tr("Are you ready?") + "\n",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No){
            return;
        }
    }
    else{
        QMessageBox::information(this, tr("Confirm"), errmsg);
        return;
    }

    // Clear the status
    ui->listWidgetStatus->clear();

    // Clear the working folder.
    eraseFilesInFolder(workingFolder);

    setCursor(Qt::WaitCursor);

    // Copy the first target file into the working folder.
    QString inputPath = ui->listWidget_EF->item(0)->text();
    //QFile *file = new QFile(fpath);
    //QFileInfo *fileinfo = new QFileInfo(fpath);
    //QString wfpath = workingFolder + "/" + fileinfo->fileName();

    // Check result of copy function
    //if(! file->copy(firstFilePath)){
    //    copyErrorPath = fpath;
    //    stat = false;
    //}

    if(stat){
        // Make MeCab Input file into the working folder.
        stat = makeMeCabInputFile(inputPath, meCabInputPath, maskColumnNo);
    }
    
    doneFileCount = 0;
    
    if(stat){
        // Execute MeCab.
        ui->listWidgetStatus->addItem(tr("Started processing."));

        pthread->meCabExecute(meCabHome, meCabBatchPath, meCabInputPath, meCabOutputPath, ui->listWidgetStatus, processExecutionTimeoutSeconds);
    }
    else{
        QMessageBox::information(this, tr("Confirm"), tr("Can't copy the target files into the working folder.")
            + "(" + copyErrorPath + ")\n\n"
            + tr("Aborted.") + "\n"
            + tr("Check free space of the disk where this program installed.") + "\n"
            + tr("About twice the free space of the target file is needed."));
    }
}

//------------------------------------------------------------------------------
bool MainWindow::editOutputFile(QString inputPath, QString meCabOutputPath, QString outputPath, QString maskColumnNo)
{
    bool stat = true;
    QString errmsg = "";
    int col = 0;
    QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    QTextDecoder *decoder = codec->makeDecoder();
    QTextEncoder *encoder = codec->makeEncoder();
    
    QFile wfile(outputPath);
    QFile rfileMeCab(meCabOutputPath);
    QFile rfile(inputPath);
    if (wfile.open(QIODevice::WriteOnly | QIODevice::Text)){
        if (rfileMeCab.open(QIODevice::ReadOnly | QIODevice::Text)){
            if(maskColumnNo != ""){
                if (rfile.open(QIODevice::ReadOnly | QIODevice::Text)){
                    bool ok;
                    col = maskColumnNo.toInt(&ok, 10);
                    if(! ok){
                        errmsg = tr("Illegal Mask Column No.") + "(" + maskColumnNo + ")";
                        rfile.close();
                        stat = false;
                    }
                }
                else{
                    // Can't open input file.
                    errmsg = tr("Can't open the intput file.") + "(" + inputPath + ")";
                    stat = false;
                }
            }
            
            if(stat){
                QString outline = "";
                while (!rfileMeCab.atEnd()) {
                    QByteArray line = rfileMeCab.readLine();
                    //QString str = decoder->toUnicode(line);
                    QString str = line;
                    if(str == "EOS\n"){
                        if(maskColumnNo != ""){
                            QByteArray wline = rfile.readLine();
                            str = wline;
                            QStringList wlist = str.split("\t", QString::KeepEmptyParts, Qt::CaseSensitive);
                            str = outline;
                            outline = "";
                            for(int ii = 0; ii < wlist.count(); ii++){
                                if(outline != ""){
                                    outline += "\t";
                                }
                                if(ii == col){
                                    outline += str;
                                }
                                else{
                                    outline += wlist[ii];
                                }
                            }
                        }
                        wfile.write((outline + "\n").toAscii());
                        outline = "";
                    }
                    else{
                        QStringList wlist = str.split("\t", QString::KeepEmptyParts, Qt::CaseSensitive);
                        if(wlist[1].left(13) == "–¼ŽŒ,ŒÅ—L–¼ŽŒ"){
                            QString unicodeStr = decoder->toUnicode(wlist[0].toAscii());
                            QString maskStr = unicodeStr.replace(QRegExp("."), decoder->toUnicode("¡"));
                            outline += encoder->fromUnicode(maskStr);
                        }
                        else{
                            outline += wlist[0];
                        }
                    }
                }
                rfileMeCab.close();
                if(maskColumnNo != ""){
                    rfile.close();
                }
            }
        }
        else{
            // Can't open MeCab output file.
            errmsg = tr("Can't open the MeCab output file.") + "(" + meCabOutputPath + ")";
            stat = false;
        }
        
        wfile.close();
    }
    else{
        // Can't open the output file.
        errmsg = tr("Can't open the output file.") + "(" + outputPath + ")";
        stat = false;
    }
    
    if(! stat){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::critical(this, tr("ERROR"),
            errmsg,
            QMessageBox::Abort);
        if (reply == QMessageBox::Abort){
            close();
        }
    }
    
    return stat;
}

//------------------------------------------------------------------------------
void MainWindow::pthreadDone()
{
    bool stat;
    QString outputFolder = ui->lineEdit_OutputFolder->text();
    QString workingFolder = "tmp";
    QString meCabBatchPath = "tmp/meCab.bat";
    QString meCabInputPath = "tmp/meCabInput.txt";
    QString meCabOutputPath = "tmp/meCabOutput.txt";
    
    if(pthread->coprocessStartErrorMsg == ""){
        stat = true;
    }
    else{
        stat = false;
    }

    if(ui->listWidgetStatus->count()){
        ui->listWidgetStatus->setCurrentItem(ui->listWidgetStatus->item(ui->listWidgetStatus->count() - 1), 0);
    }

    if(stat){
        QString inputPath = ui->listWidget_EF->item(doneFileCount)->text();
        QFileInfo *fileinfo = new QFileInfo(inputPath);
        QString outputPath = outputFolder + "/" + fileinfo->fileName() + ".msk";
        stat = editOutputFile(inputPath, meCabOutputPath, outputPath, maskColumnNo);
        
        if(stat){
            doneFileCount++;
            if(doneFileCount < ui->listWidget_EF->count()){
                QString inputPath = ui->listWidget_EF->item(doneFileCount)->text();
                stat = makeMeCabInputFile(inputPath, meCabInputPath, maskColumnNo);
                if(stat){
                    pthread->meCabExecute(meCabHome, meCabBatchPath, meCabInputPath, meCabOutputPath, ui->listWidgetStatus, processExecutionTimeoutSeconds);
                }
            }
            else{
                setCursor(Qt::ArrowCursor);
                
                QMessageBox::information(this, tr("Confirm"), tr("Completed."));
                
                // Clear the main form
                //ui->listWidget_EF->clear();
                //ui->lineEdit_OutputFolder->setText("");
                //rewriteFileStatus(ui->listWidget_EF, ui->labelFileStatus_EF);

                // Clear working folder.
                if(stat){
                    //eraseFilesInFolder(workingFolder);
                }
            }
        }
    }
    else{
        setCursor(Qt::ArrowCursor);
        
        // returned some error.
        QMessageBox::information(this, tr("ERROR"), tr("Check detail by the display status."));
    }
}

void MainWindow::on_action_Exit_triggered()
{
    close();
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, tr(formalProgramName.toAscii()) + "(" + programName + ") " + " Ver." + programVersion, tr(formalProgramName.toAscii()) + tr(" is a helper program for privacy mask using MeCab."));
}

void MainWindow::on_actionHelp_triggered()
{
    formHelp->show();
    formHelp->activateWindow();
}

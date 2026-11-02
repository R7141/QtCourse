#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->actionShowLineNumbers->setCheckable(true);
    ui->actionShowLineNumbers->setChecked(true);
    connect(ui->actionShowLineNumbers, &QAction::toggled,
            ui->plainTextEdit, &CodeEditor::setLineNumbersVisible);

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->plainTextEdit->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);

    statusLabel.setMaximumWidth(150);
    statusLabel.setText("length:"+QString::number(0)+" lines:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusLabel);

    statusCursorLabel.setMaximumWidth(150);
    statusCursorLabel.setText("Ln:"+QString::number(0)+" Col:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author=new QLabel(ui->statusbar);
    author->setText(tr("E"));
    ui->statusbar->addPermanentWidget(author);

    setCurrentFile(QString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    ui->plainTextEdit->clear();
    ui->plainTextEdit->document()->setModified(false);
    setCurrentFile(QString());
    ui->plainTextEdit->setFocus();
}

void MainWindow::openFile()
{
    const QString startPath = currentFilePath.isEmpty()
            ? QDir::homePath()
            : QFileInfo(currentFilePath).absolutePath();

    const QString fileName = QFileDialog::getOpenFileName(
                this, tr("打开文件"), startPath,
                tr("文本文件 (*.txt);;C++ 文件 (*.cpp *.h);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("无法打开文件：\n%1\n\n%2")
                             .arg(fileName, file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    const QString content = stream.readAll();
    if (stream.status() != QTextStream::Ok) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("读取文件失败：\n%1").arg(fileName));
        return;
    }

    if (!maybeSave())
        return;

    ui->plainTextEdit->setPlainText(content);
    ui->plainTextEdit->document()->setModified(false);
    setCurrentFile(fileName);
    ui->plainTextEdit->setFocus();
}

bool MainWindow::saveFile()
{
    if (currentFilePath.isEmpty())
        return saveFileAs();

    return saveToFile(currentFilePath);
}

bool MainWindow::saveFileAs()
{
    const QString suggestedName = currentFilePath.isEmpty()
            ? QDir::homePath() + QStringLiteral("/未命名.txt")
            : currentFilePath;

    const QString fileName = QFileDialog::getSaveFileName(
                this, tr("另存为"), suggestedName,
                tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;

    if (!saveToFile(fileName))
        return false;

    setCurrentFile(fileName);
    return true;
}

bool MainWindow::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存文件"),
                             tr("无法保存文件：\n%1\n\n%2")
                             .arg(fileName, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << ui->plainTextEdit->toPlainText();
    stream.flush();

    const bool writeOk = stream.status() == QTextStream::Ok
            && file.error() == QFile::NoError;
    const QString errorString = file.errorString();
    file.close();

    if (!writeOk) {
        QMessageBox::warning(this, tr("保存文件"),
                             tr("写入文件失败：\n%1\n\n%2")
                             .arg(fileName, errorString));
        return false;
    }

    ui->plainTextEdit->document()->setModified(false);
    statusBar()->showMessage(tr("文件已保存：%1")
                             .arg(QFileInfo(fileName).fileName()), 2000);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!ui->plainTextEdit->document()->isModified())
        return true;

    const QString displayName = currentFilePath.isEmpty()
            ? tr("未命名")
            : QFileInfo(currentFilePath).fileName();

    const QMessageBox::StandardButton ret = QMessageBox::warning(
                this, tr("简单文本编辑器"),
                tr("文档“%1”已修改，是否保存更改？").arg(displayName),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);

    if (ret == QMessageBox::Save)
        return saveFile();
    if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFilePath = fileName;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString displayName = currentFilePath.isEmpty()
            ? tr("未命名")
            : QFileInfo(currentFilePath).fileName();

    setWindowTitle(tr("%1[*] - 简单文本编辑器").arg(displayName));
    setWindowModified(ui->plainTextEdit->document()->isModified());
}

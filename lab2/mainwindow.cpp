#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->actionShowLineNumbers->setCheckable(true);
    ui->actionShowLineNumbers->setChecked(true);
    connect(ui->actionShowLineNumbers, &QAction::toggled,
            ui->plainTextEdit, &CodeEditor::setLineNumbersVisible);

    statusLabel.setMaximumWidth(150);
    statusLabel.setText("length:"+QString::number(0)+" lines:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusLabel);

    statusCursorLabel.setMaximumWidth(150);
    statusCursorLabel.setText("Ln:"+QString::number(0)+" Col:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author=new QLabel(ui->statusbar);
    author->setText(tr("E"));
    ui->statusbar->addPermanentWidget(author);
}

MainWindow::~MainWindow()
{
    delete ui;
}

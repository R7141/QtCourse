#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    setModal(false);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->findLineEdit, &QLineEdit::returnPressed,
            ui->findNextButton, &QPushButton::click);

    connect(ui->findNextButton, &QPushButton::clicked, this, [this]() {
        emit findRequested(ui->findLineEdit->text(),
                           ui->caseCheckBox->isChecked(), false,
                           ui->wrapCheckBox->isChecked());
    });
    connect(ui->findPreviousButton, &QPushButton::clicked, this, [this]() {
        emit findRequested(ui->findLineEdit->text(),
                           ui->caseCheckBox->isChecked(), true,
                           ui->wrapCheckBox->isChecked());
    });
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::setFindText(const QString &text)
{
    if (!text.isEmpty())
        ui->findLineEdit->setText(text);

    ui->findLineEdit->selectAll();
    ui->findLineEdit->setFocus();
}

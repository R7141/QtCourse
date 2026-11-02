#include "replacedialog.h"
#include "ui_replacedialog.h"

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);
    setModal(false);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->findLineEdit, &QLineEdit::returnPressed,
            ui->findNextButton, &QPushButton::click);
    connect(ui->replaceLineEdit, &QLineEdit::returnPressed,
            ui->replaceButton, &QPushButton::click);

    connect(ui->findNextButton, &QPushButton::clicked, this, [this]() {
        emit findRequested(ui->findLineEdit->text(),
                           ui->caseCheckBox->isChecked(), false,
                           ui->wrapCheckBox->isChecked());
    });
    connect(ui->replaceButton, &QPushButton::clicked, this, [this]() {
        emit replaceRequested(ui->findLineEdit->text(),
                              ui->replaceLineEdit->text(),
                              ui->caseCheckBox->isChecked(),
                              ui->wrapCheckBox->isChecked());
    });
    connect(ui->replaceAllButton, &QPushButton::clicked, this, [this]() {
        emit replaceAllRequested(ui->findLineEdit->text(),
                                 ui->replaceLineEdit->text(),
                                 ui->caseCheckBox->isChecked(),
                                 ui->wrapCheckBox->isChecked());
    });
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

void ReplaceDialog::setFindText(const QString &text)
{
    if (!text.isEmpty())
        ui->findLineEdit->setText(text);

    ui->findLineEdit->selectAll();
    ui->findLineEdit->setFocus();
}

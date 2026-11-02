#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

#include "importprogressdialog.h"
#include "ui_importprogressdialog.h"

ImportProgressDialog::ImportProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportProgressDialog)
{
    ui->setupUi(this);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setValue(0);
}

ImportProgressDialog::~ImportProgressDialog()
{
    delete ui;
}


void ImportProgressDialog::setNumberOfFramesToImport(int frames)
{
    ui->progressBar->setMaximum(frames);
}


void ImportProgressDialog::increment ()
{
    ui->progressBar->setValue (ui->progressBar->value() + 1);
}

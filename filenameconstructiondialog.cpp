#include "filenameconstructiondialog.h"
#include "ui_filenameconstructiondialog.h"

FilenameConstructionDialog::FilenameConstructionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilenameConstructionDialog)
{
    ui->setupUi(this);
}

FilenameConstructionDialog::~FilenameConstructionDialog()
{
    delete ui;
}

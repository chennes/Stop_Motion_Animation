#include "helpdialog.h"
#include "ui_helpdialog.h"

#include <QTextDocument>
#include <QFile>
#include <QTextCodec>

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    QFile helpFile (":/files/help.html");
    helpFile.open (QIODevice::ReadOnly);
    QByteArray fileBytes = helpFile.readAll();
    QString fileString = QTextCodec::codecForUtfText(fileBytes)->toUnicode(fileBytes);
    ui->textBrowser->setText(fileString);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}

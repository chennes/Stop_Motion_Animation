#ifndef IMPORTPROGRESSDIALOG_H
#define IMPORTPROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class ImportProgressDialog;
}

class ImportProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportProgressDialog(QWidget *parent = 0);
    ~ImportProgressDialog();

    void setNumberOfFramesToImport(int frames);

    void increment ();

private:
    Ui::ImportProgressDialog *ui;
};

#endif // IMPORTPROGRESSDIALOG_H

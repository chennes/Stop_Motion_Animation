#ifndef FILENAMECONSTRUCTIONDIALOG_H
#define FILENAMECONSTRUCTIONDIALOG_H

#include <QDialog>

namespace Ui {
class FilenameConstructionDialog;
}

class FilenameConstructionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilenameConstructionDialog(QWidget *parent = 0);
    ~FilenameConstructionDialog();

private:
    Ui::FilenameConstructionDialog *ui;
};

#endif // FILENAMECONSTRUCTIONDIALOG_H

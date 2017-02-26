#ifndef SAVEFINALMOVIEDIALOG_H
#define SAVEFINALMOVIEDIALOG_H

#include <QDialog>

namespace Ui {
class SaveFinalMovieDialog;
}

class SaveFinalMovieDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveFinalMovieDialog(QWidget *parent = 0);
    ~SaveFinalMovieDialog();

    /**
     * @brief reset clears the contents of the input boxes and recalculates a default filename. Call before show().
     */
    void reset(const QString &name);

    QString filename() const;
    QString movieTitle() const;
    QString credits() const;

private slots:

    void on_changeLocationButton_clicked();

private:
    Ui::SaveFinalMovieDialog *ui;
};

#endif // SAVEFINALMOVIEDIALOG_H

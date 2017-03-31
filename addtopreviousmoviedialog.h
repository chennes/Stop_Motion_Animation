#ifndef ADDTOPREVIOUSMOVIEDIALOG_H
#define ADDTOPREVIOUSMOVIEDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "movie.h"
#include <memory>

namespace Ui {
class AddToPreviousMovieDialog;
}

class AddToPreviousMovieDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddToPreviousMovieDialog(QWidget *parent = 0);
    ~AddToPreviousMovieDialog();

    QString getSelectedMovie() const;

public slots:

    void Reset ();

private slots:

    void on_treeWidget_itemSelectionChanged();

    void on_horizontalSlider_valueChanged(int value);

    void on_treeWidget_doubleClicked(const QModelIndex &index);

protected:
    virtual void showEvent(QShowEvent *)  ;

private:
    Ui::AddToPreviousMovieDialog *ui;
    std::map<QTreeWidgetItem *, std::shared_ptr<Movie> > _itemToMovieMap;
};

#endif // ADDTOPREVIOUSMOVIEDIALOG_H

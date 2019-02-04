#include "addtopreviousmoviedialog.h"
#include "ui_addtopreviousmoviedialog.h"

#include "settings.h"

#include <QDir>

AddToPreviousMovieDialog::AddToPreviousMovieDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddToPreviousMovieDialog)
{
    ui->setupUi(this);
}



void AddToPreviousMovieDialog::showEvent(QShowEvent *)
{
    Reset();
}


void AddToPreviousMovieDialog::Reset ()
{
    _itemToMovieMap.clear();
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem *tli = ui->treeWidget->topLevelItem(i);
        qDeleteAll (tli->takeChildren());
    }

    Settings settings;
    QString imageStorageLocation = settings.Get("settings/imageStorageLocation").toString();
    QString imageFilenameFormat = settings.Get("settings/imageFilenameFormat").toString();

    // Find all the files that match the appropriate pattern:
    // imageStorageLocation/timestamp/timestamp.json

    QDir dir (imageStorageLocation);
    dir.setSorting(QDir::Name | QDir::Reversed);
    if (dir.isReadable()) {
        auto entryList = dir.entryList(QDir::Dirs);
        for (auto entry : entryList) {
            QDateTime timestamp (QDateTime::fromString(entry, imageFilenameFormat));
            if (timestamp.isValid()) {
                QDir subdir (imageStorageLocation + entry);
                if (subdir.isReadable()) {
                    QString filename (imageStorageLocation + entry + "/" + entry + ".json");
                    std::shared_ptr<Movie> movie = std::shared_ptr<Movie> (new Movie(entry, false));
                    bool loaded = movie->load (filename);
                    if (loaded) {
                        QStringList columns;
                        QString niceDate = timestamp.toString("MMMM d");
                        if (timestamp.daysTo(QDateTime::currentDateTime()) > 365) {
                            niceDate = timestamp.date().toString(Qt::SystemLocaleLongDate);
                        }
                        QString niceTime = timestamp.toLocalTime().time().toString(Qt::SystemLocaleShortDate);
                        QString nFrames = QString::number(movie->getNumberOfFrames());
                        columns.append (niceDate);
                        columns.append (niceTime);
                        columns.append (nFrames);
                        // 0 -Today
                        // 1 - Yesterday
                        // 2 - This week (assume the week started on Monday)
                        // 3 - Last week (really, to the previous Monday)
                        // 4 - Earlier
                        int insertionItem;
                        int daysToToday = int(abs(timestamp.daysTo(QDateTime::currentDateTime())));
                        int daysToMonday = QDate::currentDate().dayOfWeek() - 1;
                        if (daysToToday <= 0) {
                            insertionItem = 0;
                        } else if (daysToToday == 1) {
                            insertionItem = 1;
                        } else if (daysToToday <= daysToMonday) {
                            insertionItem = 2;
                        } else if (daysToToday < daysToMonday+7) {
                            insertionItem = 3;
                        } else {
                            insertionItem = 4;
                        }
                        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget->topLevelItem(insertionItem), columns);
                        _itemToMovieMap[item] = movie;
                    }
                }
            }
        }
    }

    bool foundFirst = false;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem *tli = ui->treeWidget->topLevelItem(i);
        if (tli->childCount() == 0) {
            tli->setHidden (true);
        } else {
            tli->setHidden (false);
            tli->setSelected(false);
            for (int child = 0; child < tli->childCount(); child++) {
                if (!foundFirst) {
                    foundFirst = true;
                    ui->treeWidget->expandItem(tli);
                    tli->child(child)->setSelected(true);
                } else {
                    tli->child(child)->setSelected(false);
                }
            }
        }
    }

    ui->treeWidget->resizeColumnToContents(0);
    ui->treeWidget->resizeColumnToContents(1);
    ui->treeWidget->resizeColumnToContents(2);
}

AddToPreviousMovieDialog::~AddToPreviousMovieDialog()
{
    delete ui;
}

QString AddToPreviousMovieDialog::getSelectedMovie() const
{
    auto selection = ui->treeWidget->selectedItems();
    if (selection.size() == 1) {
        ui->horizontalSlider->setMinimum(1);
        auto itr = _itemToMovieMap.find (selection[0]);
        if (itr != _itemToMovieMap.end()) {
            return itr->second->getSaveFilename();
        }
    }
    return QString ();
}

void AddToPreviousMovieDialog::on_treeWidget_itemSelectionChanged()
{
    // Hook up to the movie to show the frames:
    ui->frameLabel->setPixmap(QPixmap());
    ui->frameLabel->setText("<b>Select a movie from the list on the left</b>");
    auto selection = ui->treeWidget->selectedItems();
    if (selection.size() > 0) {
        ui->horizontalSlider->setMinimum(1);
        auto movie = _itemToMovieMap[selection[0]];
        if (movie) {
            ui->horizontalSlider->setMaximum(movie->getNumberOfFrames());
            movie->setStillFrame (0, ui->frameLabel);
        }
    }
}

void AddToPreviousMovieDialog::on_horizontalSlider_valueChanged(int value)
{
    auto selection = ui->treeWidget->selectedItems();
    if (selection.size() == 1) {
        auto movie = _itemToMovieMap[selection[0]];
        if (movie) {
            movie->setStillFrame (value-1, ui->frameLabel);
        }
    }
}

void AddToPreviousMovieDialog::on_treeWidget_doubleClicked(const QModelIndex &)
{
    auto selection = ui->treeWidget->selectedItems();
    if (selection.size() > 0) {
        auto movie = _itemToMovieMap[selection[0]];
        if (movie) {
            this->accept();
        }
    }
}

#ifndef BACKGROUNDMUSICDIALOG_H
#define BACKGROUNDMUSICDIALOG_H

#include <QDialog>
#include <QString>
#include <QAudioDecoder>
#include <QGraphicsScene>

namespace Ui {
class BackgroundMusicDialog;
}

class BackgroundMusicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackgroundMusicDialog(QWidget *parent = 0);
    ~BackgroundMusicDialog();

    void loadFile (const QString &filename);


private slots:
    void on_chooseMusicFileButton_clicked();
    void readBuffer ();


private:
    Ui::BackgroundMusicDialog *ui;
    QAudioDecoder *_decoder;
    QGraphicsScene *_scene;
};

#endif // BACKGROUNDMUSICDIALOG_H

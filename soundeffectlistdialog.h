#ifndef SOUNDEFFECTLISTDIALOG_H
#define SOUNDEFFECTLISTDIALOG_H

#include <QDialog>
#include <QList>
#include "soundeffect.h"


namespace Ui {
class SoundEffectListDialog;
}

class SoundEffectListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SoundEffectListDialog( QWidget *parent = 0);
    ~SoundEffectListDialog();

    void AddSoundEffect(const SoundEffect &sfx);

    void AddSoundEffects(const QList<SoundEffect> &sfx);

    void RemoveSoundEffect(const SoundEffect &sfx);

    void RemoveAllSoundEffects ();

signals:
    void remove(const SoundEffect &sfx);
    void edit(const SoundEffect &sfx);
    void play(const SoundEffect &sfx);
    void selected (const SoundEffect &sfx);

private slots:
    void on_tableWidget_cellClicked(int row, int column);

    void on_tableWidget_cellDoubleClicked(int row, int column);

    void on_removeButton_clicked();

    void on_editButton_clicked();

    void on_playButton_clicked();

private:

    SoundEffect SFXFromRow(int row) const;

private:
    Ui::SoundEffectListDialog *ui;
    QList<SoundEffect> _sfx;
};

#endif // SOUNDEFFECTLISTDIALOG_H

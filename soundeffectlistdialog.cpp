#include "soundeffectlistdialog.h"
#include "ui_soundeffectlistdialog.h"

SoundEffectListDialog::SoundEffectListDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoundEffectListDialog)
{
    ui->setupUi(this);
    ui->playButton->setEnabled(false);
    ui->removeButton->setEnabled(false);
    ui->editButton->setEnabled(false);
}

SoundEffectListDialog::~SoundEffectListDialog()
{
    delete ui;
}

void SoundEffectListDialog::AddSoundEffect(const SoundEffect &sfx)
{
    _sfx.append(sfx);
    auto startFrame = sfx.getStartFrame();
    auto filename = sfx.getFilename();
    auto inPoint = sfx.getInPoint();
    auto outPoint = sfx.getOutPoint();
    auto duration = outPoint - inPoint;
    auto linearVolume = sfx.getVolume();
    qreal logVolume = QAudio::convertVolume(linearVolume * qreal(100.0),
                                            QAudio::LinearVolumeScale,
                                            QAudio::LogarithmicVolumeScale);

    QTableWidgetItem *newItemFrame  = new QTableWidgetItem(QString::number(startFrame)); // Frame
    QTableWidgetItem *newItemFilename   = new QTableWidgetItem(filename); // Filename
    QTableWidgetItem *newItemStart   = new QTableWidgetItem(QString::number(inPoint)); // Start
    QTableWidgetItem *newItemEnd   = new QTableWidgetItem(QString::number(outPoint)); // End
    QTableWidgetItem *newItemDuration   = new QTableWidgetItem(QString::number(duration)); // Duration
    QTableWidgetItem *newItemVolume   = new QTableWidgetItem(QString::number(logVolume)); // Volume

    auto row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    ui->tableWidget->setItem(row, 0, newItemFrame);
    ui->tableWidget->setItem(row, 1, newItemStart);
    ui->tableWidget->setItem(row, 2, newItemEnd);
    ui->tableWidget->setItem(row, 3, newItemDuration);
    ui->tableWidget->setItem(row, 4, newItemVolume);
    ui->tableWidget->setItem(row, 5, newItemFilename);
}

void SoundEffectListDialog::AddSoundEffects(const QList<SoundEffect> &sfx)
{
    for (auto &&individualSFX:sfx) {
        AddSoundEffect(individualSFX);
    }
}

void SoundEffectListDialog::RemoveSoundEffect(const SoundEffect &sfx)
{
    ui->playButton->setEnabled(false);
    ui->removeButton->setEnabled(false);
    ui->editButton->setEnabled(false);
    _sfx.removeAll(sfx);
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        if (SFXFromRow(row) == sfx) {
            ui->tableWidget->removeRow(row);
            break;
        }
    }
}

void SoundEffectListDialog::RemoveAllSoundEffects()
{
    _sfx.clear();
    while (ui->tableWidget->rowCount() > 0) {
        ui->tableWidget->removeRow(0);
    }
    ui->playButton->setEnabled(false);
    ui->removeButton->setEnabled(false);
    ui->editButton->setEnabled(false);
}

void SoundEffectListDialog::on_tableWidget_cellClicked(int row, int)
{
    ui->playButton->setEnabled(true);
    ui->removeButton->setEnabled(true);
    ui->editButton->setEnabled(true);
    emit selected (SFXFromRow(row));
}

void SoundEffectListDialog::on_tableWidget_cellDoubleClicked(int row, int)
{
    emit edit(SFXFromRow (row));
}

void SoundEffectListDialog::on_removeButton_clicked()
{
    SoundEffect sfxToRemove = SFXFromRow (ui->tableWidget->currentRow());
    RemoveSoundEffect(sfxToRemove);
    emit remove(sfxToRemove);
}

void SoundEffectListDialog::on_editButton_clicked()
{
    SoundEffect sfxToEdit = SFXFromRow (ui->tableWidget->currentRow());
    emit edit(sfxToEdit);
}

void SoundEffectListDialog::on_playButton_clicked()
{
    SoundEffect sfxToPlay = SFXFromRow (ui->tableWidget->currentRow());
    emit play(sfxToPlay);
}

SoundEffect SoundEffectListDialog::SFXFromRow(int row) const
{
    auto startFrame = ui->tableWidget->item(row, 0)->text().toInt();
    auto inPoint = ui->tableWidget->item(row, 1)->text().toDouble();
    auto outPoint = ui->tableWidget->item(row, 2)->text().toDouble();
    auto logVolume = ui->tableWidget->item(row, 4)->text().toDouble();
    auto filename = ui->tableWidget->item(row,5)->text();

    qreal linearVolume = QAudio::convertVolume(logVolume / qreal(100.0),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    SoundEffect sfx (filename, startFrame, inPoint, outPoint, linearVolume);
    return sfx;
}

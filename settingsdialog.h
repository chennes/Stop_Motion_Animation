#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "filenameconstructiondialog.h"

namespace Ui {
class SettingsDialog;
}

/**
 * @brief The SettingsDialog class
 * This dialog should be used as a modal dialog and run via its exec() function.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    /**
     * @brief load
     * Load the settings from the saved file if it exists. If not file exists, do nothing.
     */
    void load ();

    /**
     * @brief store
     * Store the current settings into the settings file. If the file does not exist, create it.
     */
    void store ();

    static QVariant getDefault (const QString &key);

protected:
    virtual void closeEvent(QCloseEvent *event);


private slots:
    void on_imageLocationBrowseButton_clicked();

    void on_filenameFormatHelpButton_clicked();

    void on_preTitleScreenFileBrowseButton_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::SettingsDialog *ui;
    FilenameConstructionDialog _filenameConstructionHelp;

    static QMap<QString, QVariant> SETTING_DEFAULTS;
};

#endif // SETTINGSDIALOG_H

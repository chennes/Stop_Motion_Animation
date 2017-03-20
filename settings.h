#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = 0);

    QVariant Get(const QString &key);

    void Set(const QString &key, QVariant value);

    void Reset ();

private:

    static QSettings::Format JSONFormat;
    static QString _settingsFile;

    static bool readJSONFile(QIODevice &device, QSettings::SettingsMap &map);

    static bool writeJSONFile(QIODevice &device, const QSettings::SettingsMap &map);

    static QMap<QString, QVariant> SETTING_DEFAULTS;
};

#endif // SETTINGS_H

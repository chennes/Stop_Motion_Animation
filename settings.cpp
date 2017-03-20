#include "settings.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QIODevice>

#include <QSize>

QSettings::Format Settings::JSONFormat;
QString Settings::_settingsFile;
QMap<QString, QVariant> Settings::SETTING_DEFAULTS;

Settings::Settings(QObject *parent) : QObject(parent)
{
    // If this is the first settings object created during this run, do some initialization
    if (SETTING_DEFAULTS.empty()) {
        SETTING_DEFAULTS.insert("settings/imageFileType","JPG");
        SETTING_DEFAULTS.insert("settings/framesPerSecond",15);
        SETTING_DEFAULTS.insert("settings/imageStorageLocation","Image Files/");
        SETTING_DEFAULTS.insert("settings/imageFilenameFormat","yyyy-MM-dd-hh-mm-ss");
        SETTING_DEFAULTS.insert("settings/camera","Always use default");
        SETTING_DEFAULTS.insert("settings/imageWidth",640);
        SETTING_DEFAULTS.insert("settings/imageHeight",480);
        SETTING_DEFAULTS.insert("settings/preTitleScreenLocation","./PreTitleScreen.jpg");
        SETTING_DEFAULTS.insert("settings/preTitleScreenDuration",2.0);
        SETTING_DEFAULTS.insert("settings/titleScreenDuration",2.0);
        SETTING_DEFAULTS.insert("settings/creditsDuration",5.0);

        JSONFormat = QSettings::registerFormat("json", Settings::readJSONFile, Settings::writeJSONFile);
        _settingsFile = "./StopMotionCreatorSettings.json";
        QSettings::setDefaultFormat(JSONFormat);
    }
}



QVariant Settings::Get(const QString &key)
{
    QSettings s(_settingsFile, JSONFormat);
    return s.value (key, SETTING_DEFAULTS[key]);
}

void Settings::Set(const QString &key, QVariant value)
{
    QSettings s(_settingsFile, JSONFormat);
    s.setValue(key, value);
}

void Settings::Reset()
{
    for (auto i = SETTING_DEFAULTS.constBegin(); i != SETTING_DEFAULTS.constEnd(); ++i) {
        Set (i.key(), i.value());
    }
}

bool Settings::readJSONFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QJsonDocument doc;
    doc.fromBinaryData(device.readAll());
    map = doc.object().toVariantMap();
    return true;
}

bool Settings::writeJSONFile(QIODevice &device, const QSettings::SettingsMap &map)
{
    QJsonObject json = QJsonObject::fromVariantMap(map);
    QJsonDocument doc;
    doc.setObject(json);
    qint64 ret = device.write (doc.toJson());
    if (ret > 0) {
        return true;
    } else {
        return false;
    }
}

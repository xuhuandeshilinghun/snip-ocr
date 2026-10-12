#pragma once
#include <QString>
#include <QVariant>
#include <QJsonObject>

class ConfigManager {
public:
    static ConfigManager& instance();

    QVariant get(const QString& key, const QVariant& defaultVal = QVariant()) const;
    void set(const QString& key, const QVariant& value);
    QJsonObject get_all() const;
    QString config_file_path() const;

private:
    ConfigManager();
    void load();
    void save();

    QString _config_path;
    QJsonObject _config;
};

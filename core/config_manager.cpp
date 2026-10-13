/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "config_manager.h"
#include "resource_helper.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager() {
    QDir base = ResourceHelper::runtime_base_dir();
    _config_path = base.filePath(".config_settings.json");
    load();
}

void ConfigManager::load() {
    
    _config = QJsonObject{
        {"theme", "dark"},
        {"language", "zh_CN"},
        {"hotkey", QJsonObject{
            {"snapshot", "F1"},
            {"pin", "F2"},
            {"ocr", "Ctrl+Shift+O"},
            {"delay_snapshot", "Ctrl+Shift+D"}
        }},
        {"delay_seconds", 5},
        {"delay_loop", false},
        {"ocr_rec_lang", "auto"},
        {"minimize_to_tray", true},
        {"auto_start", false},
        {"magnifier", QJsonObject{
            {"enabled", true},
            {"color_format", "RGB"},
            {"hide_hash", false},
            {"hex_uppercase", false}
        }},
        {"custom_colors", QJsonArray()},
        {"color_usage", QJsonObject()},
        {"groups", QJsonArray{
            QJsonObject{{"id", "default"}, {"name", "默认"}}
        }},
        {"active_group_id", "default"}
    };

    QFile f(_config_path);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll();
        f.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject loaded = doc.object();
            
            for (auto it = loaded.begin(); it != loaded.end(); ++it) {
                _config[it.key()] = it.value();
            }
        }
    }
}

void ConfigManager::save() {
    QJsonDocument doc(_config);
    QFile f(_config_path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
    }
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultVal) const {
    if (!_config.contains(key)) return defaultVal;
    QJsonValue val = _config.value(key);
    return val.toVariant();
}

void ConfigManager::set(const QString& key, const QVariant& value) {
    _config[key] = QJsonValue::fromVariant(value);
    save();
}

QJsonObject ConfigManager::get_all() const {
    return _config;
}

QString ConfigManager::config_file_path() const {
    return _config_path;
}

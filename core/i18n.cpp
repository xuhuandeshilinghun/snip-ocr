#include "i18n.h"
#include "locale_res_loader.h"
#include <QJsonObject>

namespace I18n {

static QString _current_lang = "zh_CN";
static QJsonObject _translations;
static bool _loaded = false;

static void _load_translations(const QString& lang) {
    _current_lang = lang;
    QJsonObject data = LocaleResLoader::get_locale_resource(lang);
    if (!data.isEmpty()) {
        _translations = data;
        _loaded = true;
        return;
    }
    
    if (lang != "zh_CN") {
        data = LocaleResLoader::get_locale_resource("zh_CN");
        if (!data.isEmpty()) {
            _translations = data;
            _loaded = true;
            return;
        }
    }
    
    _translations = LocaleResLoader::get_fallback_zh_cn();
    _loaded = true;
}

QString init_language(const QString& lang) {
    QString l = lang.isEmpty() ? "zh_CN" : lang;
    _load_translations(l);
    return _current_lang;
}

void set_language(const QString& lang) {
    static const QStringList supported = {
        "zh_CN", "zh_TW", "en_US", "ja", "ko",
        "de", "fr", "es", "pt", "ru"
    };
    if (supported.contains(lang)) {
        _load_translations(lang);
    }
}

QString get_current_lang() {
    return _current_lang;
}

QString t(const QString& key, const QString& default_val) {
    if (!_loaded) {
        _load_translations(_current_lang);
    }
    if (_translations.contains(key)) {
        return _translations.value(key).toString();
    }
    return default_val.isEmpty() ? key : default_val;
}

    QString t_fmt(const QString& key, const QString& default_val, const QMap<QString, QString>& params) {
        QString str = t(key, default_val);
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            str.replace("{" + it.key() + "}", it.value());
        }
        return str;
    }

} // namespace I18n

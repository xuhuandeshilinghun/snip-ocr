#pragma once
#include <QString>
#include <QJsonObject>

namespace LocaleResLoader {
    
    
    QJsonObject get_locale_resource(const QString& lang_id);
    
    QJsonObject get_fallback_zh_cn();
}

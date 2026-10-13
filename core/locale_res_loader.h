/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>
#include <QJsonObject>

namespace LocaleResLoader {
    
    
    QJsonObject get_locale_resource(const QString& lang_id);
    
    QJsonObject get_fallback_zh_cn();
}

/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>
#include <QMap>

namespace I18n {
    
    QString init_language(const QString& lang = QString());
    
    void set_language(const QString& lang);
    
    QString get_current_lang();
    
    QString t(const QString& key, const QString& default_val = QString());
}

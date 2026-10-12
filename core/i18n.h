#pragma once
#include <QString>
#include <QMap>

namespace I18n {
    
    QString init_language(const QString& lang = QString());
    
    void set_language(const QString& lang);
    
    QString get_current_lang();
    
    QString t(const QString& key, const QString& default_val = QString());
}

/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QStringList>
#include <QString>

namespace RegionRouter {
    
    QStringList get_version_urls();
    
    QString get_issues_url();
    
    
    QString get_download_base_url();
    
    QString build_download_url(const QString& version, const QString& filename);
    
    bool is_china();
}

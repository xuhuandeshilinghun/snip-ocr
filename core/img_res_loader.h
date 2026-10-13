/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>
#include <QPixmap>
#include <QIcon>
#include <QByteArray>

namespace ImgResLoader {
    
    QByteArray get_img_bytes(const QString& filename);
    
    QString get_img_path(const QString& filename);
    
    QPixmap get_img_pixmap(const QString& filename);
    
    QIcon get_img_icon(const QString& filename);
}

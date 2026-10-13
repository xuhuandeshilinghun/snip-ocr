/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>

namespace Constants {
    inline const QString I18N_FOLDER = "locales";
    inline const QString DEFAULT_LANG = "zh_CN";
    inline const QStringList SUPPORT_LANG = {"zh_CN", "en_US"};

    inline const QStringList OCR_SUPPORTED_EXTS = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tif", ".tiff", ".gif"
    };

    inline const QString DEFAULT_SNAPSHOT_HOTKEY = "F1";
    inline const QString DEFAULT_OCR_HOTKEY = "Ctrl+Shift+O";
}

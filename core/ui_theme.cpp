/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "ui_theme.h"
#include "config_manager.h"

namespace UiTheme {

bool is_dark_theme() {
    auto& cfg = ConfigManager::instance();
    QString theme = cfg.get("theme", QStringLiteral("dark")).toString();
    return theme == "dark";
}

} // namespace UiTheme

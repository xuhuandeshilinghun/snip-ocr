#include "ui_theme.h"
#include "config_manager.h"

namespace UiTheme {

bool is_dark_theme() {
    auto& cfg = ConfigManager::instance();
    QString theme = cfg.get("theme", QStringLiteral("dark")).toString();
    return theme == "dark";
}

} // namespace UiTheme

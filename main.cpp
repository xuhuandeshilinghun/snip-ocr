/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include <QApplication>
#include <QFile>
#include <QIcon>
#include "core/i18n.h"
#include "core/config_manager.h"
#include "core/ui_theme.h"
#include "core/img_res_loader.h"
#include "Snip_ocr_app.h"
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include "core/resource_helper.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstring>
#include <cstdio>
#endif

// Apply dark/light theme to QApplication (Python _apply_app_theme).
static void applyAppTheme()
{
    if (!UiTheme::is_dark_theme()) {
        qApp->setStyleSheet(QString());
        return;
    }
    qApp->setStyleSheet(
        "QWidget { background: #1e1e1e; color: #e0e0e0; }"
        "QToolTip { background: #2d2d2d; color: #e0e0e0; border: 1px solid #3d3d3d; }"
    );
}

int main(int argc, char* argv[])
{
    // Debug log - write to file immediately
    {
        FILE* dbg = fopen("main_debug.log", "a");
        if (dbg) {
            fprintf(dbg, "=== main() entered ===\n");
            fprintf(dbg, "argc=%d\n", argc);
            for (int i = 0; i < argc; i++)
                fprintf(dbg, "argv[%d]=%s\n", i, argv[i]);
            fflush(dbg);
            fclose(dbg);
        }
    }

#ifdef Q_OS_WIN
    // Single instance mutex (Global\ namespace, user-level)
    // Check for --restart flag (set by updater when restarting after update)
    bool isRestart = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--restart") == 0) {
            isRestart = true;
            break;
        }
    }

    // Single instance mutex (skip when restarting after update)
    if (!isRestart) {
        HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Global\\SnipOCR_SingleInstance_Mutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (mutex) CloseHandle(mutex);
            return 0;
        }
    }
#endif

    QApplication app(argc, argv);

    // Read configured language first (for Qt translator loading)
    auto& cfg = ConfigManager::instance();
    QString lang = cfg.get("language", "zh_CN").toString();
    QString localeName = lang;

    app.setApplicationName("SnipOCR");
    app.setQuitOnLastWindowClosed(false);

    // App icon (loaded from memory, no temp file extraction)
    QIcon appIcon = ImgResLoader::get_img_icon("logo.ico");
    if (!appIcon.isNull())
        app.setWindowIcon(appIcon);

    I18n::init_language(lang);

    applyAppTheme();

    SnipOCRApp window;
    Q_UNUSED(window);

    return app.exec();
}

/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "system_tray.h"
#include "config_manager.h"
#include "i18n.h"
#include "ui_theme.h"
#include "img_res_loader.h"
#include "resource_helper.h"

#include <QSystemTrayIcon>
#include <QPainter>
#include <QPixmap>
#include <QAction>
#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

TrayManager::TrayManager(QObject* parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(this);
    m_menu = new QMenu();
    buildMenu();
    connect(m_tray, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason r) { onActivated(static_cast<int>(r)); });
}

TrayManager::~TrayManager()
{
    delete m_menu;
}

QString TrayManager::getMenuStylesheet(bool dark) const
{
    if (dark) {
        return QStringLiteral(
            "QMenu { background: #2d2d2d; color: #ffffff; border: 1px solid #3d3d3d; padding: 4px 0; }"
            "QMenu::item { padding: 6px 20px 6px 8px; margin: 0 4px; border-radius: 4px; }"
            "QMenu::item:selected { background: #404040; }"
            "QMenu::separator { height: 1px; background: #3d3d3d; margin: 4px 8px; }"
            "QMenu::icon { margin-left: 2px; margin-right: 4px; }");
    } else {
        return QStringLiteral(
            "QMenu { background: #ffffff; color: #333333; border: 1px solid #cccccc; padding: 4px 0; }"
            "QMenu::item { padding: 6px 20px 6px 8px; margin: 0 4px; border-radius: 4px; }"
            "QMenu::item:selected { background: #e0e0e0; }"
            "QMenu::separator { height: 1px; background: #cccccc; margin: 4px 8px; }"
            "QMenu::icon { margin-left: 2px; margin-right: 4px; }");
    }
}

void TrayManager::buildMenu()
{
    bool dark = UiTheme::is_dark_theme();
    m_menu->setStyleSheet(getMenuStylesheet(dark));
    m_menu->clear();

    auto& cfg = ConfigManager::instance();
    QVariant hkVar = cfg.get("hotkey");
    QJsonObject hk = hkVar.toJsonObject();
    if (hk.isEmpty()) {
        QVariantMap vm = hkVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            hk.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    QString SnipKey = hk.value("Snipshot").toString("F1");
    QString pinKey = hk.value("pin").toString("F2");
    QString ocrKey = hk.value("ocr").toString("Ctrl+Shift+O");
    QString delayKey = hk.value("delay_Snipshot").toString("Ctrl+Shift+D");

    // Screenshot
    auto* actSnip = new QAction(I18n::t("tray.Snipshot", QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe")) +
                                QStringLiteral(" (%1)").arg(SnipKey), m_menu);
    connect(actSnip, &QAction::triggered, this, &TrayManager::screenshotRequested);
    m_menu->addAction(actSnip);

    auto* actPin = new QAction(I18n::t("tray.pin", QStringLiteral("\xe8\xb4\xb4\xe5\x9b\xbe")) +
                               QStringLiteral(" (%1)").arg(pinKey), m_menu);
    connect(actPin, &QAction::triggered, this, &TrayManager::pinRequested);
    m_menu->addAction(actPin);

    // OCR
    auto* actOcr = new QAction(I18n::t("tray.ocr", QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbeOCR")) +
                               QStringLiteral(" (%1)").arg(ocrKey), m_menu);
    connect(actOcr, &QAction::triggered, this, &TrayManager::ocrRequested);
    m_menu->addAction(actOcr);

    // Delay
    auto* actDelay = new QAction(I18n::t("tray.delay_screenshot", QStringLiteral("\xe5\xbb\xb6\xe6\x97\xb6\xe6\x88\xaa\xe5\x9b\xbe")) +
                                 QStringLiteral(" (%1)").arg(delayKey), m_menu);
    connect(actDelay, &QAction::triggered, this, &TrayManager::delayRequested);
    m_menu->addAction(actDelay);

    m_menu->addSeparator();

    // Groups submenu
    buildGroupsSubmenu();

    
    auto* actClearHistory = new QAction(I18n::t("tray.clear_history", QStringLiteral("\xe6\xb8\x85\xe7\xa9\xba\xe6\x88\xaa\xe5\xb1\x8f\xe5\x8e\x86\xe5\x8f\xb2")), m_menu);
    connect(actClearHistory, &QAction::triggered, this, &TrayManager::clearHistoryRequested);
    m_menu->addAction(actClearHistory);

    
    auto* actDisableHotkey = new QAction(I18n::t("tray.disable_hotkey", QStringLiteral("\xe7\xa6\x81\xe7\x94\xa8\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae")), m_menu);
    actDisableHotkey->setCheckable(true);
    actDisableHotkey->setChecked(cfg.get("hotkey_disabled", false).toBool());
    connect(actDisableHotkey, &QAction::toggled, this, [this](bool checked) {
        ConfigManager::instance().set("hotkey_disabled", checked);
        updateTrayIcon();
        emit hotkeyToggleRequested(checked);
    });
    m_menu->addAction(actDisableHotkey);

    m_menu->addSeparator();

    // Preferences submenu
    auto* prefsMenu = new QMenu(I18n::t("tray.preferences", QStringLiteral("\xe9\xa6\x96\xe9\x80\x89\xe9\xa1\xb9")), m_menu);
    prefsMenu->setStyleSheet(m_menu->styleSheet());
    auto* actSettings = new QAction(I18n::t("tray.settings", QStringLiteral("\xe8\xae\xbe\xe7\xbd\xae...")), prefsMenu);
    connect(actSettings, &QAction::triggered, this, &TrayManager::settingsRequested);
    prefsMenu->addAction(actSettings);
    auto* actAbout = new QAction(I18n::t("tray.about", QStringLiteral("\xe5\x85\xb3\xe4\xba\x8e")), prefsMenu);
    connect(actAbout, &QAction::triggered, this, &TrayManager::aboutRequested);
    prefsMenu->addAction(actAbout);
    prefsMenu->addSeparator();
    auto* actUpdate = new QAction(I18n::t("tray.check_update", QStringLiteral("\xe6\xa3\x80\xe6\x9f\xa5\xe6\x9b\xb4\xe6\x96\xb0")), prefsMenu);
    connect(actUpdate, &QAction::triggered, this, &TrayManager::updateCheckRequested);
    prefsMenu->addAction(actUpdate);
    m_menu->addMenu(prefsMenu);

    m_menu->addSeparator();

    
    auto* actRestart = new QAction(I18n::t("tray.restart", QStringLiteral("\xe9\x87\x8d\xe6\x96\xb0\xe5\x90\xaf\xe5\x8a\xa8")), m_menu);
    connect(actRestart, &QAction::triggered, this, &TrayManager::restartRequested);
    m_menu->addAction(actRestart);

    // Exit
    auto* actExit = new QAction(I18n::t("tray.exit", QStringLiteral("\xe9\x80\x80\xe5\x87\xba")), m_menu);
    connect(actExit, &QAction::triggered, this, &TrayManager::exitRequested);
    m_menu->addAction(actExit);
}

void TrayManager::buildGroupsSubmenu()
{
    auto* groupsMenu = new QMenu(I18n::t("tray.groups", QStringLiteral("\xe8\xb4\xb4\xe5\x9b\xbe\xe7\xbb\x84")), m_menu);
    groupsMenu->setStyleSheet(m_menu->styleSheet());
    connect(groupsMenu, &QMenu::aboutToShow, this, [this, groupsMenu]() { refreshGroupsMenu(groupsMenu); });
    m_menu->addMenu(groupsMenu);
}

void TrayManager::refreshGroupsMenu(QMenu* menu)
{
    menu->clear();
    auto& cfg = ConfigManager::instance();
    QJsonArray groups = cfg.get("groups").toJsonArray();
    if (groups.isEmpty()) {
        QJsonObject def;
        def["id"] = "default";
        def["name"] = "default";
        groups.append(def);
    }
    QString activeId = cfg.get("active_group_id", "default").toString();

    QDir base = ResourceHelper::runtime_base_dir();
    for (const QJsonValue& g : groups) {
        QJsonObject go = g.toObject();
        QString gid = go.value("id").toString();
        QString gname = go.value("name").toString(gid);
        if (gid == "default")
            gname = I18n::t("tray.group_default_name", gname);
        // Count screenshots in group
        int count = 0;
        QDir gdir = base.filePath("history/" + gid);
        if (gdir.exists()) {
            QStringList files = gdir.entryList(QStringList() << "sp*.png", QDir::Files);
            count = files.size();
        }
        QString label = QStringLiteral("%1  %2").arg(gname).arg(count);
        auto* act = new QAction(label, menu);
        if (gid == activeId) {
            act->setIcon(ImgResLoader::get_img_icon("checkbox_checked.svg"));
        }
        QString capturedGid = gid;
        connect(act, &QAction::triggered, this, [this, capturedGid]() {
            emit groupSwitchRequested(capturedGid);
        });
        menu->addAction(act);
    }

    menu->addSeparator();
    auto* actNew = new QAction(I18n::t("tray.new_group", QStringLiteral("\xe5\x88\x9b\xe5\xbb\xba\xe6\x96\xb0\xe7\xbb\x84...")), menu);
    connect(actNew, &QAction::triggered, this, &TrayManager::groupCreateRequested);
    menu->addAction(actNew);
}

void TrayManager::onActivated(int reason)
{
    m_lastTrayPos = QCursor::pos();
    if (reason == static_cast<int>(QSystemTrayIcon::Trigger)) {
        // Single left-click
        emit screenshotRequested();
    } else if (reason == static_cast<int>(QSystemTrayIcon::DoubleClick)) {
        emit showMainRequested();
    } else if (reason == static_cast<int>(QSystemTrayIcon::Context)) {
        showContextMenu();
    }
}

void TrayManager::showContextMenu()
{
    QPoint pos = QCursor::pos();
    int iconRight = pos.x() + 16;
    int iconTop = pos.y() - 16;
    int menuH = m_menu->sizeHint().height();
    if (menuH <= 0) menuH = 160;
    int menuX = iconRight;
    int menuY = iconTop - menuH;
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        if (menuX + m_menu->sizeHint().width() > geo.right())
            menuX = geo.right() - m_menu->sizeHint().width();
        if (menuY < geo.top())
            menuY = iconTop + 16;
    }
    m_menu->popup(QPoint(menuX, menuY));
}

QPoint TrayManager::getTrayPosition() const
{
    if (!m_lastTrayPos.isNull())
        return m_lastTrayPos;
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        return QPoint(geo.right() - 40, geo.bottom() - 20);
    }
    return QPoint();
}

QString TrayManager::buildTooltipText() const
{
    auto& cfg = ConfigManager::instance();
    QVariant hkVar = cfg.get("hotkey");
    QJsonObject hk = hkVar.toJsonObject();
    if (hk.isEmpty()) {
        QVariantMap vm = hkVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            hk.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    QString SnipKey = hk.value("Snipshot").toString("F1");
    QString ocrKey = hk.value("ocr").toString("Ctrl+Shift+O");
    QString title = I18n::t("app.title", "SnipOCR");
    QString SnipLabel = I18n::t("tray.screenshot_hotkey", "Screenshot hotkey");
    QString ocrLabel = I18n::t("tray.ocr_hotkey", "OCR hotkey");
    QString freeLabel = I18n::t("tray.free_version", "Free");
    
    QString modelPath = QDir(QCoreApplication::applicationDirPath()).filePath("model/MediaOCR-Base-v1.mtk-model");
    bool hasModel = QFileInfo::exists(modelPath);
    if (hasModel) {
        return QStringLiteral("%1 V1.0.0 %2\n%3: %4\n%5: %6")
            .arg(title).arg(freeLabel)
            .arg(SnipLabel).arg(SnipKey)
            .arg(ocrLabel).arg(ocrKey);
    }
    return QStringLiteral("%1 V1.0.0 %2\n%3: %4")
        .arg(title).arg(freeLabel)
        .arg(SnipLabel).arg(SnipKey);
}

void TrayManager::showIcon()
{
    m_tray->show();
    // Tooltip must be set AFTER show() on Windows, otherwise the NIF_TIP flag
    // is not applied to the shell notification icon.
    updateTrayIcon();
    m_tray->setToolTip(buildTooltipText());
}

void TrayManager::updateTrayIcon()
{
    QIcon baseIcon = ImgResLoader::get_img_icon("logo.ico");
    if (baseIcon.isNull()) {
        m_tray->setIcon(QIcon());
        return;
    }

    bool disabled = ConfigManager::instance().get("hotkey_disabled", false).toBool();
    if (!disabled) {
        m_tray->setIcon(baseIcon);
        return;
    }

    
    const int sz = 32;
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    
    baseIcon.paint(&p, 0, 0, sz, sz);

    
    int badgeR = 7;          
    int badgeX = sz - badgeR - 1;  
    int badgeY = sz - badgeR - 1;

    
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xD8, 0x04, 0x05));  // #D80405
    p.drawEllipse(QPoint(badgeX, badgeY), badgeR, badgeR);

    
    int barW = 8;   
    int barH = 2;   
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawRect(QRect(badgeX - barW / 2, badgeY - barH / 2, barW, barH));

    p.end();
    m_tray->setIcon(QIcon(pm));
}

void TrayManager::hideIcon()
{
    m_tray->hide();
}

void TrayManager::notify(const QString& message, const QString& title, int duration)
{
    m_tray->showMessage(title, message, QSystemTrayIcon::Information, duration);
}

void TrayManager::refreshMenu()
{
    buildMenu();
}

void TrayManager::refreshTooltip()
{
    m_tray->setToolTip(buildTooltipText());
}

void TrayManager::refreshTheme()
{
    bool dark = UiTheme::is_dark_theme();
    QString style = getMenuStylesheet(dark);
    m_menu->setStyleSheet(style);
    updateSubmenusStyle(m_menu, style);
}

void TrayManager::updateSubmenusStyle(QMenu* menu, const QString& style)
{
    for (QAction* act : menu->actions()) {
        QMenu* submenu = act->menu();
        if (submenu) {
            submenu->setStyleSheet(style);
            updateSubmenusStyle(submenu, style);
        }
    }
}

void TrayManager::cleanup()
{
    m_tray->hide();
}

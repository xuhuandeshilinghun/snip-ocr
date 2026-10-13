/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QObject>
#include <QMenu>
#include <QPoint>
#include <QIcon>

class QSystemTrayIcon;
class QAction;

// System tray manager. Left-click -> screenshot; right-click -> menu.
// Faithfully ports Python core/system_tray.py.
class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);
    ~TrayManager() override;

    void showIcon();
    void hideIcon();
    void notify(const QString& message, const QString& title = "SnipOCR", int duration = 3000);
    void refreshMenu();
    void refreshTooltip();
    void refreshTheme();
    void updateTrayIcon();
    QPoint getTrayPosition() const;
    void cleanup();

signals:
    void screenshotRequested();
    void pinRequested();
    void ocrRequested();
    void delayRequested();
    void settingsRequested();
    void exitRequested();
    void showMainRequested();
    void aboutRequested();
    void updateCheckRequested();
    void groupSwitchRequested(const QString& groupId);
    void groupCreateRequested();
    void clearHistoryRequested();
    void hotkeyToggleRequested(bool disabled);
    void restartRequested();

private:
    void buildMenu();
    void buildGroupsSubmenu();
    void refreshGroupsMenu(QMenu* menu);
    QString getMenuStylesheet(bool dark) const;
    void onActivated(int reason);
    void showContextMenu();
    QString buildTooltipText() const;
    void updateSubmenusStyle(QMenu* menu, const QString& style);

    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
    QPoint m_lastTrayPos;
};

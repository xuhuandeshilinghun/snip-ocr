/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QDialog>
#include <QHash>
#include <QSet>
#include <QString>

class QTabWidget;
class QComboBox;
class QCheckBox;
class QKeySequenceEdit;
class QPushButton;
class QLabel;
class GlobalHotkeyManager;

// Settings dialog (Snipaste-style, tabbed).
// Faithfully ports Python ui/settings_dialog.py.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

    void setHotkeyManager(GlobalHotkeyManager* mgr);

signals:
    void settingsChanged();

private slots:
    void onSave();
    void onCancel();
    void onTabChanged(int index);
    void onThemeChanged();
    void onRestoreMagDefaults();
    void onHotkeyChanged();

private:
    void buildUi();
    void buildGeneralTab();
    void buildControlTab();
    void buildMagnifierTab();
    void buildButtonRow();
    void initPixmaps();
    void loadValues();
    void applyTheme(bool dark = false);
    void validateHotkeys();
    QPair<bool, QString> checkConflicts() const;
    void centerOnScreen();

    // Auto-start (Windows Startup folder shortcut)
    static QString getStartupDir();
    static QString getShortcutPath();
    static bool createStartupShortcut();
    static bool removeStartupShortcut();
    static bool isAutoStartEnabled();
    static void syncAutoStart(bool enabled);

    // General tab
    QComboBox* m_comboLang = nullptr;
    QComboBox* m_comboTheme = nullptr;
    QCheckBox* m_checkTray = nullptr;
    QCheckBox* m_checkAutostart = nullptr;

    // Control tab - behavior sub-tab
    QComboBox* m_comboDelay = nullptr;
    QCheckBox* m_checkDelayLoop = nullptr;
    QLabel* m_labelDelayHint = nullptr;
    QComboBox* m_comboOcrLang = nullptr;
    QLabel* m_labelOcrHint = nullptr;

    // Control tab - hotkey sub-tab
    QKeySequenceEdit* m_editSnap = nullptr;
    QLabel* m_iconSnap = nullptr;
    QKeySequenceEdit* m_editPin = nullptr;
    QLabel* m_iconPin = nullptr;
    QKeySequenceEdit* m_editOcr = nullptr;
    QLabel* m_iconOcr = nullptr;
    QKeySequenceEdit* m_editDelay = nullptr;
    QLabel* m_iconDelay = nullptr;

    // Magnifier tab
    QComboBox* m_comboMagColor = nullptr;
    QCheckBox* m_checkMagEnabled = nullptr;
    QCheckBox* m_checkMagHideHash = nullptr;
    QCheckBox* m_checkMagHexUpper = nullptr;

    // Main tabs
    QTabWidget* m_tabs = nullptr;
    QTabWidget* m_subTabs = nullptr;

    // Buttons
    QPushButton* m_btnSave = nullptr;
    QPushButton* m_btnCancel = nullptr;

    // OK/Fail pixmaps
    QPixmap m_pixmapOk;
    QPixmap m_pixmapFail;

    GlobalHotkeyManager* m_hotkeyMgr = nullptr;

    // System hotkey conflicts set (e.g. "Ctrl+C", "Alt+F4")
    static QSet<QString> s_systemConflicts;
};

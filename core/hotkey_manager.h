/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QObject>
#include <QString>
#include <QTimer>

// Forward-declare Windows types so we don't need <windows.h> in the header.
// On x64, LRESULT = long long, WPARAM = unsigned __int64, LPARAM = long long.
// We use a simple signature compatible with HOOKPROC on x64 ABI.
class GlobalHotkeyManager : public QObject {
    Q_OBJECT
public:
    explicit GlobalHotkeyManager(QObject* parent = nullptr);
    ~GlobalHotkeyManager() override;

    // Returns true on success.
    bool start();
    void stop();
    void setEnabled(bool enabled);
    void reloadHotkeys();

signals:
    void SnipshotTriggered();
    void pinTriggered();
    void ocrTriggered();
    void delayTriggered();

private:
    void loadHotkeys();
    static int parseSingleVk(const QString& key);
    static int parseCombo(const QString& key, int& outVk);

    void* m_hook = nullptr;   // HHOOK
    bool m_enabled = true;

    int m_SnipshotMods = 0;
    int m_SnipshotVk = 0;
    int m_pinMods = 0;
    int m_pinVk = 0;
    int m_ocrMods = 0;
    int m_ocrVk = 0;
    int m_delayMods = 0;
    int m_delayVk = 0;

    static GlobalHotkeyManager* s_instance;
    // Low-level keyboard hook callback.
    // Implementation uses LRESULT/WPARAM/LPARAM types from <windows.h>.
    // Cast to HOOKPROC via reinterpret_cast in start().
    static long long hookProc(int nCode, unsigned int wParam, long long lParam);
};

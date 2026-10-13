/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "hotkey_manager.h"
#include "config_manager.h"
#include "i18n.h"

#include <windows.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>

// Modifier bit flags (Python parse_combo).
// NB: VK_*/WM_*/HC_*/WH_*/LLKHF_*/KBDLLHOOKSTRUCT all come from <windows.h>.
// Renamed to HOTMOD_* to avoid clash with winuser.h MOD_ALT/MOD_CONTROL/MOD_SHIFT/MOD_WIN.
static constexpr int HOTMOD_CTRL  = 0x0002;
static constexpr int HOTMOD_SHIFT = 0x0004;
static constexpr int HOTMOD_ALT   = 0x0001;
static constexpr int HOTMOD_WIN   = 0x0008;

// Diagnostic log file (Snip-text_C/hotkey_debug.log) for verifying hook installation.
static void hotkeyLog(const QString& msg)
{
    static QString logPath;
    if (logPath.isEmpty()) {
        logPath = QDir::currentPath() + QStringLiteral("/hotkey_debug.log");
    }
    QFile f(logPath);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        f.write(QStringLiteral("[%1] %2\n").arg(ts).arg(msg).toUtf8());
    }
}

GlobalHotkeyManager* GlobalHotkeyManager::s_instance = nullptr;

GlobalHotkeyManager::GlobalHotkeyManager(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
    loadHotkeys();
    hotkeyLog(QStringLiteral("GlobalHotkeyManager constructed: SnipVk=%1 pinVk=%2 ocrVk=%3 delayVk=%4")
                .arg(m_SnipshotVk).arg(m_pinVk).arg(m_ocrVk).arg(m_delayVk));
}

GlobalHotkeyManager::~GlobalHotkeyManager()
{
    stop();
    if (s_instance == this)
        s_instance = nullptr;
}

static int namedVk(const QString& up) {
    
    if (up == "VOLUMEMUTE" || up == "VOLUME_MUTE") return VK_VOLUME_MUTE;   // 0xAD
    if (up == "VOLUMEDOWN" || up == "VOLUME_DOWN") return VK_VOLUME_DOWN;   // 0xAE
    if (up == "VOLUMEUP" || up == "VOLUME_UP") return VK_VOLUME_UP;         // 0xAF
    if (up == "MEDIANEXT" || up == "MEDIA_NEXT") return VK_MEDIA_NEXT_TRACK;
    if (up == "MEDIAPREVIOUS" || up == "MEDIA_PREVIOUS") return VK_MEDIA_PREV_TRACK;
    if (up == "MEDIASTOP" || up == "MEDIA_STOP") return VK_MEDIA_STOP;
    if (up == "MEDIAPLAY" || up == "MEDIA_PLAY") return VK_MEDIA_PLAY_PAUSE;
    if (up == "LAUNCHMAIL" || up == "LAUNCH_MAIL") return VK_LAUNCH_MAIL;
    if (up == "LAUNCHAPP1" || up == "LAUNCH_APP1") return VK_LAUNCH_APP1;
    if (up == "LAUNCHAPP2" || up == "LAUNCH_APP2") return VK_LAUNCH_APP2;
    if (up == "BROWSERBACK" || up == "BROWSER_BACK") return VK_BROWSER_BACK;
    if (up == "BROWSERFORWARD" || up == "BROWSER_FORWARD") return VK_BROWSER_FORWARD;
    if (up == "BROWSERREFRESH" || up == "BROWSER_REFRESH") return VK_BROWSER_REFRESH;
    if (up == "BROWSERSTOP" || up == "BROWSER_STOP") return VK_BROWSER_STOP;
    if (up == "BROWSERSEARCH" || up == "BROWSER_SEARCH") return VK_BROWSER_SEARCH;
    if (up == "BROWSERFAVORITES" || up == "BROWSER_FAVORITES") return VK_BROWSER_FAVORITES;
    if (up == "BROWSERHOME" || up == "BROWSER_HOME") return VK_BROWSER_HOME;
    if (up == "ESCAPE" || up == "ESC") return VK_ESCAPE;
    if (up == "TAB") return VK_TAB;
    if (up == "BACKSPACE" || up == "BACK") return VK_BACK;
    if (up == "RETURN" || up == "ENTER") return VK_RETURN;
    if (up == "SPACE") return VK_SPACE;
    if (up == "INSERT") return VK_INSERT;
    if (up == "DELETE" || up == "DEL") return VK_DELETE;
    if (up == "HOME") return VK_HOME;
    if (up == "END") return VK_END;
    if (up == "PAGEUP") return VK_PRIOR;
    if (up == "PAGEDOWN") return VK_NEXT;
    if (up == "LEFT") return VK_LEFT;
    if (up == "RIGHT") return VK_RIGHT;
    if (up == "UP") return VK_UP;
    if (up == "DOWN") return VK_DOWN;
    if (up == "PRINT" || up == "PRINTSCREEN" || up == "SYSRQ") return VK_SnipSHOT;
    if (up == "SCROLLLOCK" || up == "SCROLL") return VK_SCROLL;
    if (up == "PAUSE" || up == "BREAK") return VK_PAUSE;
    if (up == "NUMLOCK") return VK_NUMLOCK;
    if (up == "CAPSLOCK" || up == "CAPS") return VK_CAPITAL;
    return 0;
}

int GlobalHotkeyManager::parseSingleVk(const QString& key)
{
    QString s = key.trimmed();
    QString up = s.toUpper();
    
    QString upNoSpace = up.remove(' ');
    int nv = namedVk(upNoSpace);
    if (nv == 0) nv = namedVk(up);
    if (nv != 0) return nv;
    if (up.startsWith('F') && up.mid(1).toInt() > 0) {
        bool ok = false;
        int n = up.mid(1).toInt(&ok);
        if (ok) return 0x6F + n;
    }
    if (s.length() == 1)
        return s.at(0).toLatin1();
    return 0;
}

int GlobalHotkeyManager::parseCombo(const QString& key, int& outVk)
{
    int mods = 0;
    outVk = 0;
    const auto parts = key.trimmed().split('+');
    for (const QString& p : parts) {
        QString pu = p.trimmed().toUpper();
        if (pu == "CTRL" || pu == "CONTROL")
            mods |= HOTMOD_CTRL;
        else if (pu == "SHIFT")
            mods |= HOTMOD_SHIFT;
        else if (pu == "ALT")
            mods |= HOTMOD_ALT;
        else if (pu == "WIN")
            mods |= HOTMOD_WIN;
        else {
            QString puNoSpace = pu;
            puNoSpace.remove(' ');
            int nv = namedVk(puNoSpace);
            if (nv == 0) nv = namedVk(pu);
            if (nv != 0) {
                outVk = nv;
            } else if (pu.startsWith('F') && pu.mid(1).toInt() > 0) {
                bool ok = false;
                int n = pu.mid(1).toInt(&ok);
                if (ok) outVk = 0x6F + n;
            } else if (p.length() == 1) {
                outVk = p.at(0).toLatin1();
            }
        }
    }
    return mods;
}

void GlobalHotkeyManager::loadHotkeys()
{
    auto& cfg = ConfigManager::instance();
    QVariant hkVar = cfg.get("hotkey");
    QJsonObject hk = hkVar.toJsonObject();
    if (hk.isEmpty()) {
        // Try as QVariantMap
        QVariantMap vm = hkVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            hk.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }

    // Snipshot key: supports combo (Ctrl+K) or single key (F1)
    QString SnipStr = hk.value("Snipshot").toString("F1");
    if (SnipStr.contains('+')) {
        m_SnipshotMods = parseCombo(SnipStr, m_SnipshotVk);
    } else {
        m_SnipshotMods = 0;
        m_SnipshotVk = parseSingleVk(SnipStr);
    }

    QString pinStr = hk.value("pin").toString("F2");
    if (pinStr.contains('+')) {
        m_pinMods = parseCombo(pinStr, m_pinVk);
    } else {
        m_pinMods = 0;
        m_pinVk = parseSingleVk(pinStr);
    }

    QString ocrStr = hk.value("ocr").toString("Ctrl+Shift+O");
    m_ocrMods = parseCombo(ocrStr, m_ocrVk);

    QString delayStr = hk.value("delay_Snipshot").toString("Ctrl+Shift+D");
    m_delayMods = parseCombo(delayStr, m_delayVk);
}

bool GlobalHotkeyManager::start()
{
    if (m_hook)
        return true; // already running

    HMODULE hMod = GetModuleHandleW(nullptr);
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(GlobalHotkeyManager::hookProc),
        hMod, 0);
    if (!m_hook) {
        DWORD err = GetLastError();
        hotkeyLog(QStringLiteral("SetWindowsHookExW FAILED GLE=%1").arg(err));
        qWarning() << "SetWindowsHookExW failed, GLE=" << err;
        return false;
    }
    hotkeyLog(QStringLiteral("SetWindowsHookExW OK hMod=0x%1 hHook=0x%2")
                .arg(reinterpret_cast<quintptr>(hMod), 0, 16)
                .arg(reinterpret_cast<quintptr>(m_hook), 0, 16));
    return true;
}

void GlobalHotkeyManager::stop()
{
    if (m_hook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_hook));
        m_hook = nullptr;
        hotkeyLog(QStringLiteral("Hook unhooked"));
    }
}

void GlobalHotkeyManager::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void GlobalHotkeyManager::reloadHotkeys()
{
    loadHotkeys();
    hotkeyLog(QStringLiteral("Hotkeys reloaded: SnipVk=%1 pinVk=%2 ocrVk=%3 delayVk=%4")
                .arg(m_SnipshotVk).arg(m_pinVk).arg(m_ocrVk).arg(m_delayVk));
}

long long GlobalHotkeyManager::hookProc(int nCode, unsigned int wParam, long long lParam)
{
    // On x64 the real HOOKPROTOC passes WPARAM as 8 bytes; we read the low 32 bits
    // which is sufficient for WM_KEYDOWN/WM_SYSKEYDOWN and our vk comparison.
    if (nCode == HC_ACTION && s_instance && s_instance->m_enabled) {
        auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        int vk = static_cast<int>(kb->vkCode);
        bool isKeyUp = (kb->flags & LLKHF_UP) != 0;
        if (isKeyUp) {
            return CallNextHookEx(nullptr, nCode,
                static_cast<unsigned long long>(wParam),
                static_cast<long long>(lParam));
        }
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        if (!isKeyDown) {
            return CallNextHookEx(nullptr, nCode,
                static_cast<unsigned long long>(wParam),
                static_cast<long long>(lParam));
        }

        // Helper to check modifier state
        auto checkMods = [](int mods) -> bool {
            if (mods & HOTMOD_CTRL) {
                if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
            }
            if (mods & HOTMOD_SHIFT) {
                if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) return false;
            }
            if (mods & HOTMOD_ALT) {
                if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
            }
            if (mods & HOTMOD_WIN) {
                if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) &&
                    !(GetAsyncKeyState(VK_RWIN) & 0x8000)) return false;
            }
            return true;
        };

        // Snipshot
        if (s_instance->m_SnipshotVk != 0 && vk == s_instance->m_SnipshotVk) {
            if (checkMods(s_instance->m_SnipshotMods)) {
                hotkeyLog(QStringLiteral("Snipshot triggered vk=%1").arg(vk));
                QTimer::singleShot(0, s_instance, []() {
                    if (s_instance) emit s_instance->SnipshotTriggered();
                });
                return 1;
            }
        }
        // Pin
        if (s_instance->m_pinVk != 0 && vk == s_instance->m_pinVk) {
            if (checkMods(s_instance->m_pinMods)) {
                hotkeyLog(QStringLiteral("pin triggered vk=%1").arg(vk));
                QTimer::singleShot(0, s_instance, []() {
                    if (s_instance) emit s_instance->pinTriggered();
                });
                return 1;
            }
        }
        // OCR
        if (s_instance->m_ocrVk != 0 && vk == s_instance->m_ocrVk) {
            if (checkMods(s_instance->m_ocrMods)) {
                hotkeyLog(QStringLiteral("ocr triggered vk=%1").arg(vk));
                QTimer::singleShot(0, s_instance, []() {
                    if (s_instance) emit s_instance->ocrTriggered();
                });
                return 1;
            }
        }
        // Delay
        if (s_instance->m_delayVk != 0 && vk == s_instance->m_delayVk) {
            if (checkMods(s_instance->m_delayMods)) {
                hotkeyLog(QStringLiteral("delay triggered vk=%1").arg(vk));
                QTimer::singleShot(0, s_instance, []() {
                    if (s_instance) emit s_instance->delayTriggered();
                });
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode,
        static_cast<unsigned long long>(wParam),
        static_cast<long long>(lParam));
}

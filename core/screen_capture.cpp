/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "screen_capture.h"
#include "config_manager.h"
#include "i18n.h"
#include "ui_theme.h"

#include <opencv2/opencv.hpp>
#include <QApplication>
#include <QScreen>
#include <QEventLoop>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QStringList>
#include <QtMath>
#include <QDebug>
#include <QClipboard>
#include <QSvgRenderer>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>
#include <QList>
#include <QDir>
#include <QFileInfo>
#include <climits>

#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace snap {

// ===================== Win32 Window Enumeration =====================

struct EnumCtx {
    QVector<WinRectInfo>* results;
};

// Recursively enumerate child windows (buttons, panels, etc.) like Snipaste
static BOOL CALLBACK childEnumProc(HWND hwnd, LPARAM lparam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return TRUE;
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    if (w < 3 || h < 3) return TRUE;  // Lower threshold for small controls
    auto* ctx = reinterpret_cast<EnumCtx*>(lparam);
    ctx->results->append({(void*)hwnd, QRect(rect.left, rect.top, w, h)});
    // Recurse into children to find deeper controls (buttons, text fields, etc.)
    EnumChildWindows(hwnd, childEnumProc, lparam);
    return TRUE;
}

static BOOL CALLBACK topEnumProc(HWND hwnd, LPARAM lparam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    RECT rect = {};
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    if (hr != S_OK) {
        if (!GetWindowRect(hwnd, &rect)) return TRUE;
    }
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    if (w <= 0 || h <= 0) return TRUE;
    auto* ctx = reinterpret_cast<EnumCtx*>(lparam);
    ctx->results->append({(void*)hwnd, QRect(rect.left, rect.top, w, h)});
    EnumChildWindows(hwnd, childEnumProc, lparam);
    return TRUE;
}

QVector<WinRectInfo> enumerateWindowRects() {
    QVector<WinRectInfo> results;
    EnumCtx ctx{&results};
    EnumWindows(topEnumProc, reinterpret_cast<LPARAM>(&ctx));
    return results;
}

// ===================== System Cursor Management =====================

static HCURSOR g_savedCursors[8] = {};
static bool g_cursorsReplaced = false;
static const int CURSOR_IDS[] = {32512, 32513, 32515, 32646, 32642, 32643, 32644, 32645};

void ScreenPreviewWin::replaceSystemCursorsWithCross() {
    if (g_cursorsReplaced) return;
    HCURSOR cross = LoadCursor(nullptr, IDC_CROSS);
    for (int i = 0; i < 8; i++) {
        HCURSOR orig = LoadCursor(nullptr, MAKEINTRESOURCE(CURSOR_IDS[i]));
        g_savedCursors[i] = CopyIcon(orig);
        SetSystemCursor(CopyIcon(cross), CURSOR_IDS[i]);
    }
    g_cursorsReplaced = true;
}

void ScreenPreviewWin::restoreSystemCursors() {
    if (!g_cursorsReplaced) return;
    for (int i = 0; i < 8; i++) {
        if (g_savedCursors[i]) {
            SetSystemCursor(CopyIcon(g_savedCursors[i]), CURSOR_IDS[i]);
            DestroyIcon(g_savedCursors[i]);
            g_savedCursors[i] = nullptr;
        }
    }
    g_cursorsReplaced = false;
}

// ===================== CaptureToolbar =====================

QHash<QString, QString> CaptureToolbar::iconSvgs() {
    return {
        {"pin", R"(<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M16 12V4h1V2H7v2h1v8l-2 2v2h5.2v6h1.6v-6H18v-2l-2-2z" fill="#000000"/></svg>)"},
        {"copy", R"(<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z" fill="#000000"/></svg>)"},
        {"save", R"(<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zM14 9H5V5h9v4z" fill="#000000"/></svg>)"},
        {"cancel", R"(<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" fill="#000000"/></svg>)"},
        {"qr", R"(<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M3 3h8v8H3V3zm2 2v4h4V5H5zm8-2h8v8h-8V3zm2 2v4h4V5h-4zM3 13h8v8H3v-8zm2 2v4h4v-4H5zm8-2h2v2h-2v-2zm4 0h2v2h-2v-2zm-4 4h2v2h-2v-2zm2 2h2v2h-2v-2zm2-2h2v2h-2v-2zm0-4h2v2h-2v-2zm2 4h2v2h-2v-2z" fill="#000000"/></svg>)"},
    };
}

QVector<QPair<QString, QString>> CaptureToolbar::buttonDefs() {
    // Check if model exists - hide OCR button if no model
    QString modelPath = QDir(QCoreApplication::applicationDirPath()).filePath("model/MediaOCR-Base-v1.mtk-model");
    bool hasModel = QFileInfo::exists(modelPath);

    QVector<QPair<QString, QString>> defs = {
        {"pin", ""},
        {"qr", "QR"},
        {"copy", ""},
        {"save", ""},
        {"cancel", ""},
    };
    if (hasModel) {
        defs.insert(1, {"ocr", "OCR"});
    }
    return defs;
}

QIcon CaptureToolbar::renderSvgIcon(const QString& svgStr, const QColor& color, int size) {
    QSvgRenderer renderer(svgStr.toUtf8());
    QImage img(size, size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p1(&img);
    renderer.render(&p1);
    p1.end();

    QImage result(size, size, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QPainter p2(&result);
    p2.drawImage(0, 0, img);
    p2.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p2.fillRect(result.rect(), color);
    p2.end();
    return QIcon(QPixmap::fromImage(result));
}

void CaptureToolbar::refreshIcons(const QColor& color) {
    m_iconCache.clear();
    auto svgs = iconSvgs();
    for (auto it = svgs.constBegin(); it != svgs.constEnd(); ++it) {
        m_iconCache[it.key()] = renderSvgIcon(it.value(), color);
    }
    for (auto it = m_buttons.constBegin(); it != m_buttons.constEnd(); ++it) {
        if (m_iconCache.contains(it.key())) {
            it.value()->setIcon(m_iconCache[it.key()]);
            it.value()->setIconSize(QSize(18, 18));
        }
    }
}

void CaptureToolbar::buildUi() {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    auto defs = buttonDefs();
    auto svgs = iconSvgs();
    for (const auto& def : defs) {
        const QString& key = def.first;
        const QString& label = def.second;
        auto* btn = new QToolButton();
        btn->setText(label);
        btn->setFixedSize(36, 32);
        btn->setAutoRaise(true);
        btn->setToolTip(I18n::t("capture." + key, label));

        CaptureAction action;
        if (key == "pin") action = CaptureAction::PIN;
        else if (key == "ocr") action = CaptureAction::OCR;
        else if (key == "qr") action = CaptureAction::QR;
        else if (key == "copy") action = CaptureAction::COPY;
        else if (key == "save") action = CaptureAction::SAVE;
        else action = CaptureAction::CANCEL;

        connect(btn, &QToolButton::clicked, this, [this, action]() { emit actionTriggered(action); });

        if (svgs.contains(key)) {
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        } else {
            btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        }

        m_layout->addWidget(btn);
        m_buttons[key] = btn;
    }
}

void CaptureToolbar::paintEvent(QPaintEvent*) {
    // Draw a fully opaque background so the underlying screenshot content
    // never bleeds through (no transparent corners regardless of theme).
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(m_bgColor);
    p.drawRect(rect());

    // Subtle 1px border on top of the opaque background.
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(m_borderColor, 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

void CaptureToolbar::applyStyle() {
    m_dark = UiTheme::is_dark_theme();
    m_bgColor     = m_dark ? QColor(45, 45, 45)    : QColor(255, 255, 255);
    m_borderColor = m_dark ? QColor(61, 61, 61)    : QColor(204, 204, 204);
    m_hoverColor  = m_dark ? QColor(64, 64, 64)    : QColor(224, 224, 224);
    m_textColor   = m_dark ? QColor(255, 255, 255) : QColor(51, 51, 51);

    refreshIcons(m_textColor);

    // Only style the buttons; the QFrame background is drawn in paintEvent so
    // the toolbar is always fully opaque (no transparent corners from border-radius).
    setStyleSheet(QString(
        "QToolButton { background: transparent; border: none; border-radius: 4px; color: %1; font-size: 13px; padding: 2px; }"
        "QToolButton:hover { background: %2; }"
        "QToolButton:pressed { background: %2; }"
    ).arg(m_textColor.name(QColor::HexArgb), m_hoverColor.name(QColor::HexArgb)));

    // Force a repaint when the theme changes.
    update();
}

CaptureToolbar::CaptureToolbar(QWidget* parent) : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
    buildUi();
    applyStyle();
    setCursor(Qt::ArrowCursor);
    adjustSize();
    hide();
}

void CaptureToolbar::reposition(const QRect& selRect, const QRect& screenGeo) {
    int toolbarW = sizeHint().width();
    int toolbarH = sizeHint().height();
    const int gap = 6;
    int x = selRect.right() - toolbarW;
    int y = selRect.bottom() + gap;
    if (y + toolbarH > screenGeo.bottom()) {
        y = selRect.top() - toolbarH - gap;
    }
    if (x < screenGeo.left()) {
        x = selRect.left();
    }
    move(x, y);
    adjustSize();
}

// ===================== ScreenPreviewWin =====================

ScreenPreviewWin::ScreenPreviewWin(const QPixmap& srcPix, const QRect& screenGeo,
                                   const QVector<WinRectInfo>& winRects, int screenIndex,
                                   QList<ScreenPreviewWin*>* allWins)
    : m_srcPix(srcPix), m_screenGeo(screenGeo), m_screenIndex(screenIndex), m_allWins(allWins)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setGeometry(screenGeo);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::StrongFocus);

    loadMagnifierConfig();

    // Convert win32 physical rects to local logical coords
    const int screenLeft = screenGeo.left();
    const int screenTop = screenGeo.top();
    const qreal dpr = devicePixelRatioF();
    const int SHADOW = 12;
    const LONG WS_CAPTION_FLAG = 0x00C00000;

    for (const auto& wr : winRects) {
        int ll = qRound((wr.rect.x() - screenLeft * dpr) / dpr);
        int tt = qRound((wr.rect.y() - screenTop * dpr) / dpr);
        int rr = qRound((wr.rect.right() - screenLeft * dpr) / dpr);
        int bb = qRound((wr.rect.bottom() - screenTop * dpr) / dpr);

        // Captionless windows (stickers, image popups) -> subtract shadow margin
        LONG style = GetWindowLongPtrW((HWND)wr.hwnd, GWL_STYLE);
        if (!(style & WS_CAPTION_FLAG)) {
            ll += SHADOW;
            tt += SHADOW;
            rr -= SHADOW;
            bb -= SHADOW;
            if (rr - ll <= 0 || bb - tt <= 0) continue;
        }

        QRect local(ll, tt, rr - ll, bb - tt);
        if (local.isValid() && local.intersects(rect())) {
            m_winRects.append({wr.hwnd, local});
        }
    }

    m_toolbar = new CaptureToolbar(this);
    connect(m_toolbar, &CaptureToolbar::actionTriggered, this, &ScreenPreviewWin::onToolbarAction);
}

ScreenPreviewWin::~ScreenPreviewWin() {
    restoreSystemCursors();
}

void ScreenPreviewWin::loadMagnifierConfig() {
    QVariant magVar = ConfigManager::instance().get("magnifier");
    QJsonObject mag = magVar.toJsonObject();
    if (mag.isEmpty()) {
        QVariantMap vm = magVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            mag.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    m_magVisible = mag.value("enabled").toBool(true);
    QString fmt = mag.value("color_format").toString("RGB");
    if (fmt == "RGB" || fmt == "HEX" || fmt == "RGB+HEX" || fmt == "HEX+RGB" || fmt == QString::fromUtf8("\xe6\x97\xa0")) {
        m_colorFmt = fmt;
    }
    m_hideHash = mag.value("hide_hash").toBool(false);
    m_hexUpper = mag.value("hex_uppercase").toBool(false);
}

void ScreenPreviewWin::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    forceForeground();
    setFocus(Qt::OtherFocusReason);
    replaceSystemCursorsWithCross();
    // Disable Win11 automatic rounded corners to prevent transparent border
    // around the screen-overlay capture window (DWMWA_WINDOW_CORNER_PREFERENCE=33).
    HWND captureHwnd = (HWND)winId();
    if (captureHwnd) {
        int cornerPref = 1;  // DWMWCP_DONOTROUND
        DwmSetWindowAttribute(captureHwnd, 33, &cornerPref, sizeof(cornerPref));
    }
    setCursor(Qt::CrossCursor);
}

void ScreenPreviewWin::closeEvent(QCloseEvent* e) {
    restoreSystemCursors();
    QWidget::closeEvent(e);
}

void ScreenPreviewWin::forceForeground() {
    HWND hwnd = (HWND)winId();
    DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    DWORD curThread = GetCurrentThreadId();
    if (fgThread != curThread) {
        AttachThreadInput(fgThread, curThread, TRUE);
        SetForegroundWindow(hwnd);
        AttachThreadInput(fgThread, curThread, FALSE);
    } else {
        SetForegroundWindow(hwnd);
    }
}

// ===================== Painting =====================

void ScreenPreviewWin::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e)
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    p.drawPixmap(rect(), m_srcPix);

    QColor overlay(0, 0, 0, 120);
    int w = width();
    int h = height();

    if ((m_hasSelection || m_isDragging) && !m_selRect.isNull() && m_selRect.width() > 0) {
        QRect r = m_selRect;
        if (r.top() > 0)
            p.fillRect(0, 0, w, r.top(), overlay);
        if (r.bottom() < h - 1)
            p.fillRect(0, r.bottom() + 1, w, h - r.bottom() - 1, overlay);
        if (r.left() > 0)
            p.fillRect(0, r.top(), r.left(), r.height(), overlay);
        if (r.right() < w - 1)
            p.fillRect(r.right() + 1, r.top(), w - r.right() - 1, r.height(), overlay);

        p.setPen(QPen(QColor(26, 174, 255, 220), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);

        QString szText = QString("%1 x %2").arg(r.width()).arg(r.height());
        QFont f = font(); f.setPointSize(9);
        p.setFont(f);
        p.setPen(QPen(QColor(255, 255, 255, 200), 1));
        p.drawText(r.topLeft() - QPoint(0, 18), szText);

        if (m_hasSelection) {
            int s = 8;
            p.setBrush(QColor(26, 174, 255, 220));
            p.setPen(QPen(QColor(255, 255, 255), 1));
            QRect handles[] = {
                QRect(r.left() - s/2, r.top() - s/2, s, s),
                QRect(r.right() - s/2, r.top() - s/2, s, s),
                QRect(r.left() - s/2, r.bottom() - s/2, s, s),
                QRect(r.right() - s/2, r.bottom() - s/2, s, s),
                QRect(r.center().x() - s/2, r.top() - s/2, s, s),
                QRect(r.center().x() - s/2, r.bottom() - s/2, s, s),
                QRect(r.left() - s/2, r.center().y() - s/2, s, s),
                QRect(r.right() - s/2, r.center().y() - s/2, s, s),
            };
            for (const auto& hr : handles)
                p.drawRect(hr);
        }
    } else {
        p.fillRect(rect(), overlay);

        if (!m_isDragging) {
            if (!m_snapRect.isNull()) {
                QRect r = m_snapRect;
                if (r.top() > 0)
                    p.fillRect(0, 0, w, r.top(), overlay);
                if (r.bottom() < h - 1)
                    p.fillRect(0, r.bottom() + 1, w, h - r.bottom() - 1, overlay);
                if (r.left() > 0)
                    p.fillRect(0, r.top(), r.left(), r.height(), overlay);
                if (r.right() < w - 1)
                    p.fillRect(r.right() + 1, r.top(), w - r.right() - 1, r.height(), overlay);

                p.setPen(QPen(QColor(26, 174, 255, 240), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRect(r);

                QString szText = QString("%1 x %2").arg(r.width()).arg(r.height());
                QFont f = font(); f.setPointSize(9);
                p.setFont(f);
                p.setPen(QPen(QColor(255, 255, 255, 220), 1));
                p.drawText(r.topLeft() - QPoint(0, 18), szText);
            } else {
                p.setPen(QPen(QColor(255, 255, 255, 160), 1, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRect(rect().adjusted(0, 0, -1, -1));

                QFont hf = font(); hf.setPointSize(12);
                p.setFont(hf);
                p.setPen(QColor(255, 255, 255, 200));
                p.drawText(rect(), Qt::AlignCenter, I18n::t("capture.drag_hint", QString::fromUtf8("\xe6\x8b\x96\xe6\x8b\xbd\xe9\x80\x89\xe6\x8b\xa9\xe6\x88\xaa\xe5\x9b\xbe\xe5\x8c\xba\xe5\x9f\x9f")));
            }
        }
    }

    // Magnifier: always show when visible and mouse position is valid (matching Python)
    if (m_magVisible && !m_mousePos.isNull()) {
        drawMagnifier(p);
    }
}

// ===================== Magnifier =====================

void ScreenPreviewWin::drawMagnifier(QPainter& p) {
    if (m_srcImage.isNull()) {
        m_srcImage = m_srcPix.toImage();
    }
    const int ms = m_magSize;
    const int zoom = m_magZoom;
    const int srcSize = ms / zoom;
    int mx = m_mousePos.x() - srcSize / 2;
    int my = m_mousePos.y() - srcSize / 2;
    mx = qBound(0, mx, width() - srcSize);
    my = qBound(0, my, height() - srcSize);

    int magX = m_mousePos.x() + 20;
    int magY = m_mousePos.y() + 20;
    if (magX + ms > width()) magX = m_mousePos.x() - ms - 20;
    if (magY + ms > height()) magY = m_mousePos.y() - ms - 20;
    if (magX < 0) magX = 0;
    if (magY < 0) magY = 0;

    p.save();

    // Magnifier background
    p.setBrush(QColor(30, 30, 30, 240));
    p.setPen(QPen(QColor(255, 255, 255, 200), 2));
    p.drawRoundedRect(QRect(magX, magY, ms, ms), 6, 6);

    qreal dpr = devicePixelRatioF();
    QRect srcRect(qRound(mx * dpr), qRound(my * dpr),
                  qRound(srcSize * dpr), qRound(srcSize * dpr));
    QRect dstRect(magX + 1, magY + 1, ms - 2, ms - 2);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(dstRect, m_srcImage, srcRect);

    // Crosshair
    int cx = dstRect.center().x();
    int cy = dstRect.center().y();
    p.setPen(QPen(QColor(0, 0, 0, 180), 1));
    p.drawLine(cx, dstRect.top(), cx, dstRect.bottom());
    p.drawLine(dstRect.left(), cy, dstRect.right(), cy);
    p.setPen(QPen(QColor(255, 255, 255, 120), 1));
    p.drawLine(cx + 1, dstRect.top(), cx + 1, dstRect.bottom());
    p.drawLine(dstRect.left(), cy + 1, dstRect.right(), cy + 1);

    // Color info panel
    int px = qRound(m_mousePos.x() * dpr);
    int py = qRound(m_mousePos.y() * dpr);
    if (px >= 0 && px < m_srcImage.width() && py >= 0 && py < m_srcImage.height()) {
        QColor c = m_srcImage.pixelColor(px, py);
        int r = c.red(), g = c.green(), b = c.blue();

        // Format hex string
        QString hexStr = QString("#%1%2%3")
            .arg(r, 2, 16, QChar('0'))
            .arg(g, 2, 16, QChar('0'))
            .arg(b, 2, 16, QChar('0'));
        hexStr = m_hexUpper ? hexStr.toUpper() : hexStr.toLower();
        if (m_hideHash) hexStr = hexStr.mid(1);

        QString rgbStr = QString("%1, %2, %3").arg(r).arg(g).arg(b);

        QPoint globalPos = m_mousePos + geometry().topLeft();
        QString coordStr = QString("(%1, %2)").arg(globalPos.x()).arg(globalPos.y());

        QFont lf = font(); lf.setPointSize(9);
        p.setFont(lf);
        QFontMetrics fm(lf);

        bool isNone = (m_colorFmt == QString::fromUtf8("\xe6\x97\xa0"));

        if (isNone) {
            // Coordinates only, no color info
            int infoH = 28;
            int infoY = magY + ms;
            if (infoY + infoH > height()) infoY = magY - infoH;

            p.setBrush(QColor(30, 30, 30, 240));
            p.setPen(QPen(QColor(255, 255, 255, 200), 2));
            p.drawRoundedRect(QRect(magX, infoY, ms, infoH), 6, 6);

            p.setPen(QColor(255, 255, 255));
            p.drawText(QRect(magX + 8, infoY + 5, ms - 16, 18), Qt::AlignLeft, coordStr);
        } else {
            // Determine color lines based on format
            QStringList colorLines;
            bool isCombo = (m_colorFmt == "RGB+HEX" || m_colorFmt == "HEX+RGB");
            if (isCombo) {
                if (m_colorFmt == "RGB+HEX") {
                    colorLines << rgbStr << hexStr;
                } else {
                    colorLines << hexStr << rgbStr;
                }
            } else {
                colorLines << (m_colorFmt == "HEX" ? hexStr : rgbStr);
            }

            // Determine next format for switch hint
            QString nextFmt;
            if (m_colorFmt == "RGB") nextFmt = "HEX";
            else if (m_colorFmt == "HEX") nextFmt = "RGB";
            else if (m_colorFmt == "RGB+HEX") nextFmt = "HEX+RGB";
            else nextFmt = "RGB+HEX";

            QString copyHint;
            QString switchHint;
            copyHint = I18n::t("capture.copy_color_hint", "Press C %1")
                .arg(I18n::t("capture.copy_color", "Copy color"));
            switchHint = I18n::t("capture.switch_format_hint", "Press Shift to switch to %1")
                .arg(nextFmt);

            // Auto-adjust info panel width
            int textW = fm.horizontalAdvance(coordStr);
            for (const QString& cs : colorLines)
                textW = qMax(textW, fm.horizontalAdvance(cs));
            textW = qMax(textW, fm.horizontalAdvance(copyHint));
            textW = qMax(textW, fm.horizontalAdvance(switchHint));
            int infoW = qMax(ms, textW + 30 + 16);

            int infoH = 26 + colorLines.size() * 22 + 22 * 2 + 4;
            int infoY = magY + ms;
            if (infoY + infoH > height()) infoY = magY - infoH;

            // Adjust magX if info panel would go off-screen right
            if (infoW > ms && magX + infoW > width()) {
                magX = qMax(0, width() - infoW);
            }

            p.setBrush(QColor(30, 30, 30, 240));
            p.setPen(QPen(QColor(255, 255, 255, 200), 2));
            p.drawRoundedRect(QRect(magX, infoY, infoW, infoH), 6, 6);

            // Coordinates
            p.setPen(QColor(255, 255, 255));
            p.drawText(QRect(magX + 8, infoY + 5, infoW - 16, 18), Qt::AlignLeft, coordStr);

            // Color values with swatch
            for (int i = 0; i < colorLines.size(); i++) {
                int yOff = infoY + 26 + i * 22;
                QRect swatchRect(magX + 8, yOff, 16, 16);
                p.setBrush(QColor(r, g, b));
                p.setPen(QPen(QColor(255, 255, 255, 200), 1));
                p.drawRect(swatchRect);
                p.setPen(QColor(255, 255, 255));
                p.drawText(QRect(magX + 30, yOff, infoW - 40, 18), Qt::AlignLeft, colorLines[i]);
            }

            // Hints
            int hintY = infoY + 26 + colorLines.size() * 22;
            p.setPen(QColor(180, 180, 180));
            p.drawText(QRect(magX + 8, hintY, infoW - 16, 18), Qt::AlignLeft, copyHint);
            p.drawText(QRect(magX + 8, hintY + 22, infoW - 16, 18), Qt::AlignLeft, switchHint);
        }
    }
    p.restore();
}

// ===================== Mouse Events =====================

void ScreenPreviewWin::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        closeAll();
    }
}

void ScreenPreviewWin::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    QPoint pos = e->pos();

    // Check resize handles
    if (m_hasSelection) {
        Handle h = hitTest(pos);
        if (h != None && h != Move) {
            m_resizeHandle = h;
            m_moveStart = pos;
            m_moveOrig = m_selRect;
            updateCursor(pos);
            return;
        }
    }

    // Move existing selection
    if (m_hasSelection && m_selRect.contains(pos)) {
        m_movingSelection = true;
        m_moveStart = pos;
        setCursor(Qt::SizeAllCursor);
        return;
    }

    // Start new drag selection
    if (m_hasSelection) {
        m_hasSelection = false;
        m_toolbar->hide();
    }

    m_startPt = pos;
    m_endPt = pos;
    m_isDragging = true;
    m_dragMoved = false;
    // Re-detect snap rect at click position (don't clear it)
    m_snapRect = findSnapRect(pos);
    m_selRect = QRect(m_startPt, m_endPt).normalized();
    setCursor(Qt::CrossCursor);
    update();
}

void ScreenPreviewWin::mouseMoveEvent(QMouseEvent* e) {
    m_mousePos = e->pos();
    QPoint pos = e->pos();

    // Window snapping (only when not dragging and no selection)
    if (!m_isDragging && !m_hasSelection) {
        m_snapRect = findSnapRect(pos);
        setCursor(Qt::CrossCursor);
        update();
        return;
    }

    // During drag: threshold check, then free-crop
    if (m_isDragging) {
        int dx = pos.x() - m_startPt.x();
        int dy = pos.y() - m_startPt.y();
        if (qAbs(dx) > 4 || qAbs(dy) > 4) {
            m_dragMoved = true;
            m_snapRect = QRect();
        }
        if (m_dragMoved) {
            m_endPt = pos;
            m_selRect = QRect(m_startPt, m_endPt).normalized();
        }
        update();
        return;
    }

    // Resizing selection
    if (m_hasSelection && m_resizeHandle != None) {
        QPoint delta = pos - m_moveStart;
        QRect r = m_moveOrig;
        switch (m_resizeHandle) {
            case TopLeft:     r.setTopLeft(r.topLeft() + delta); break;
            case Top:         r.setTop(r.top() + delta.y()); break;
            case TopRight:    r.setTopRight(r.topRight() + delta); break;
            case Right:       r.setRight(r.right() + delta.x()); break;
            case BottomRight: r.setBottomRight(r.bottomRight() + delta); break;
            case Bottom:      r.setBottom(r.bottom() + delta.y()); break;
            case BottomLeft:  r.setBottomLeft(r.bottomLeft() + delta); break;
            case Left:        r.setLeft(r.left() + delta.x()); break;
            default: break;
        }
        snapSelectionToEdges(r, m_resizeHandle);  // Snipaste-style edge snapping
        if (r.width() >= 5 && r.height() >= 5) {
            m_selRect = r.normalized();
        }
        m_moveStart = pos;
        update();
        m_toolbar->reposition(m_selRect, rect());
        return;
    }

    // Moving selection
    if (m_movingSelection) {
        int dx = pos.x() - m_moveStart.x();
        int dy = pos.y() - m_moveStart.y();
        QRect newRect = m_selRect.translated(dx, dy);
        snapSelectionToEdges(newRect, None);  // Snipaste-style edge snapping
        if (newRect.left() >= 0 && newRect.top() >= 0 &&
            newRect.right() < width() && newRect.bottom() < height()) {
            m_selRect = newRect;
        }
        m_moveStart = pos;
        update();
        m_toolbar->reposition(m_selRect, rect());
        return;
    }

    // Update cursor when hovering over selection
    if (m_hasSelection) {
        updateCursor(pos);
        update();
    }
}

void ScreenPreviewWin::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;

    // Finish resizing
    if (m_hasSelection && m_resizeHandle != None) {
        m_resizeHandle = None;
        setCursor(Qt::CrossCursor);
        showToolbar();
        return;
    }

    // Finish moving selection
    if (m_movingSelection) {
        m_movingSelection = false;
        setCursor(Qt::CrossCursor);
        showToolbar();
        return;
    }

    if (!m_isDragging) return;

    m_isDragging = false;

    // Click (no drag movement) with snap rect -> select snapped window
    if (!m_dragMoved && !m_snapRect.isNull()) {
        m_selRect = m_snapRect;
        m_hasSelection = true;
        m_startPt = m_selRect.topLeft();
        m_endPt = m_selRect.bottomRight();
        m_snapRect = QRect();
        setCursor(Qt::CrossCursor);
        showToolbar();
        update();
        return;
    }

    // Drag -> free crop selection
    if (m_selRect.width() < 5 || m_selRect.height() < 5) {
        m_hasSelection = false;
        m_selRect = QRect();
        closeAll();
        return;
    }

    m_hasSelection = true;
    m_snapRect = QRect();
    setCursor(Qt::CrossCursor);
    showToolbar();
    update();
}

void ScreenPreviewWin::keyPressEvent(QKeyEvent* e) {
    // Tab: toggle snap mode (window <-> ui)
    if (e->key() == Qt::Key_Tab) {
        m_snapMode = (m_snapMode == "window") ? "ui" : "window";
        m_snapRect = QRect();
        update();
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Escape) {
        closeAll();
    } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (m_hasSelection) finishCapture(CaptureAction::COPY);
    } else if (e->key() == Qt::Key_C && !m_mousePos.isNull() && m_colorFmt != QString::fromUtf8("\xe6\x97\xa0")) {
        copyColorValue();
        closeAll();
    } else if (e->key() == Qt::Key_Shift && m_colorFmt != QString::fromUtf8("\xe6\x97\xa0")) {
        // Toggle color format
        if (m_colorFmt == "RGB") m_colorFmt = "HEX";
        else if (m_colorFmt == "HEX") m_colorFmt = "RGB";
        else if (m_colorFmt == "RGB+HEX") m_colorFmt = "HEX+RGB";
        else if (m_colorFmt == "HEX+RGB") m_colorFmt = "RGB+HEX";
        update();
    } else if (e->key() == Qt::Key_M) {
        m_magVisible = !m_magVisible;
        update();
    }
}

// ===================== Handle Tests =====================

ScreenPreviewWin::Handle ScreenPreviewWin::hitTest(const QPoint& pos) const {
    const int hs = 8;
    const QRect& r = m_selRect;
    if (!r.isValid()) return None;

    if (r.adjusted(hs, hs, -hs, -hs).contains(pos)) return Move;
    if (QRect(r.left()-hs, r.top()-hs, hs*2, hs*2).contains(pos)) return TopLeft;
    if (QRect(r.right()-hs, r.top()-hs, hs*2, hs*2).contains(pos)) return TopRight;
    if (QRect(r.left()-hs, r.bottom()-hs, hs*2, hs*2).contains(pos)) return BottomLeft;
    if (QRect(r.right()-hs, r.bottom()-hs, hs*2, hs*2).contains(pos)) return BottomRight;
    if (QRect(r.left(), r.top()-hs, r.width(), hs*2).contains(pos)) return Top;
    if (QRect(r.left(), r.bottom()-hs, r.width(), hs*2).contains(pos)) return Bottom;
    if (QRect(r.left()-hs, r.top(), hs*2, r.height()).contains(pos)) return Left;
    if (QRect(r.right()-hs, r.top(), hs*2, r.height()).contains(pos)) return Right;
    return None;
}

void ScreenPreviewWin::updateCursor(const QPoint& pos) {
    Handle h = hitTest(pos);
    switch (h) {
        case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
        case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
        case Left: case Right: setCursor(Qt::SizeHorCursor); break;
        case Move: setCursor(Qt::SizeAllCursor); break;
        default: setCursor(Qt::CrossCursor); break;
    }
}

// Snipaste-style edge snapping: snap selection edges to nearby window edges
// when resizing or moving the selection. Threshold = 8 px (configurable).
void ScreenPreviewWin::snapSelectionToEdges(QRect& r, Handle handle) {
    const int threshold = 8;
    // Collect candidate edges from all window rects + screen boundaries
    QVector<int> xEdges, yEdges;
    for (const auto& wr : m_winRects) {
        xEdges.append(wr.rect.left());
        xEdges.append(wr.rect.right() + 1);
        yEdges.append(wr.rect.top());
        yEdges.append(wr.rect.bottom() + 1);
    }
    // Also snap to screen boundaries
    xEdges.append(0);
    xEdges.append(width());
    yEdges.append(0);
    yEdges.append(height());

    auto snapVal = [&](int val, const QVector<int>& edges) -> int {
        int best = val;
        int bestDist = threshold;
        for (int e : edges) {
            int d = qAbs(val - e);
            if (d < bestDist) {
                bestDist = d;
                best = e;
            }
        }
        return best;
    };

    // Determine which edges to snap based on handle
    bool snapLeft = (handle == TopLeft || handle == Left || handle == BottomLeft ||
                     handle == None);  // None = moving whole selection
    bool snapRight = (handle == TopRight || handle == Right || handle == BottomRight ||
                      handle == None);
    bool snapTop = (handle == TopLeft || handle == Top || handle == TopRight ||
                    handle == None);
    bool snapBottom = (handle == BottomLeft || handle == Bottom || handle == BottomRight ||
                       handle == None);

    if (snapLeft) {
        int newVal = snapVal(r.left(), xEdges);
        if (newVal != r.left()) r.setLeft(newVal);
    }
    if (snapRight) {
        int newVal = snapVal(r.right(), xEdges);
        if (newVal != r.right()) r.setRight(newVal);
    }
    if (snapTop) {
        int newVal = snapVal(r.top(), yEdges);
        if (newVal != r.top()) r.setTop(newVal);
    }
    if (snapBottom) {
        int newVal = snapVal(r.bottom(), yEdges);
        if (newVal != r.bottom()) r.setBottom(newVal);
    }
}

// ===================== Window Snapping =====================

QRect ScreenPreviewWin::findSnapRect(const QPoint& pos) {
    // Throttle
    if ((pos - m_lastSnapPos).manhattanLength() < 2) {
        return m_snapRect;
    }
    m_lastSnapPos = pos;

    // In "ui" mode: edge detection only
    if (m_snapMode == "ui") {
        return edgeSnap(pos);
    }

    // Snipaste-style: use WindowFromPoint for precise control detection
    // Convert local pos to global screen coordinates
    QPoint globalPos = pos + geometry().topLeft();
    POINT pt = { globalPos.x(), globalPos.y() };

    // Temporarily disable this window WS_EX_TRANSPARENT so WindowFromPoint finds what is under us
    HWND myHwnd = (HWND)winId();
    LONG_PTR oldExStyle = GetWindowLongPtrW(myHwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(myHwnd, GWL_EXSTYLE, oldExStyle | WS_EX_TRANSPARENT);

    // Find the real window/control under the cursor
    HWND target = WindowFromPoint(pt);

    // Restore our window style
    SetWindowLongPtrW(myHwnd, GWL_EXSTYLE, oldExStyle);

    if (target && target != myHwnd) {
        // Try to get the deepest child (buttons, text fields, etc.)
        HWND child = target;
        HWND prev = nullptr;
        do {
            prev = child;
            child = RealChildWindowFromPoint(child, pt);
        } while (child && child != prev && IsWindowVisible(child));

        HWND useHwnd = (child && IsWindowVisible(child)) ? child : target;
        RECT r;
        HRESULT hr = DwmGetWindowAttribute(useHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));
        if (hr != S_OK) {
            if (!GetWindowRect(useHwnd, &r)) r = {0,0,0,0};
        }
        QRect winRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
        // Convert to local coordinates
        QRect localRect = winRect.translated(-geometry().topLeft());
        if (localRect.isValid() && localRect.width() >= 3 && localRect.height() >= 3 &&
            localRect.width() < width() && localRect.height() < height()) {
            return localRect;
        }
    }

    // Fallback: use pre-enumerated window list (smallest containing)
    QRect best;
    int bestArea = INT_MAX;
    for (const auto& wr : m_winRects) {
        if (wr.rect.contains(pos)) {
            int area = wr.rect.width() * wr.rect.height();
            if (area < bestArea) {
                bestArea = area;
                best = wr.rect;
            }
        }
    }
    if (best.isValid() && best.width() > 5 && best.height() > 5) {
        return best;
    }

    // Final fallback: edge detection
    return edgeSnap(pos);
}

QRect ScreenPreviewWin::edgeSnap(const QPoint& pos) {
    if (m_srcImage.isNull()) m_srcImage = m_srcPix.toImage();
    if (m_srcImage.isNull()) return QRect();

    // Convert to ARGB32 if needed (BGRA in memory on little-endian)
    if (m_srcImage.format() != QImage::Format_ARGB32) {
        m_srcImage = m_srcImage.convertToFormat(QImage::Format_ARGB32);
    }

    qreal dpr = devicePixelRatioF();
    int px = qRound(pos.x() * dpr);
    int py = qRound(pos.y() * dpr);
    if (px < 0 || px >= m_srcImage.width() || py < 0 || py >= m_srcImage.height())
        return QRect();

    // Sample region around cursor
    const int R = 600;
    int x0 = qMax(0, px - R);
    int y0 = qMax(0, py - R);
    int x1 = qMin(m_srcImage.width(), px + R);
    int y1 = qMin(m_srcImage.height(), py + R);
    int wRegion = x1 - x0;
    int hRegion = y1 - y0;
    if (wRegion < 20 || hRegion < 20) return QRect();

    QImage subImg = m_srcImage.copy(x0, y0, wRegion, hRegion);
    cv::Mat mat(subImg.height(), subImg.width(), CV_8UC4, (void*)subImg.constBits(), subImg.bytesPerLine());

    // BGRA -> gray
    cv::Mat gray;
    cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);

    // Adaptive thresholds based on local contrast
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    double contrast = stddev[0];
    int lo = qMax(20, (int)(contrast * 0.5));
    int hi = qMax(60, (int)(contrast * 1.5));

    cv::Mat edges;
    cv::Canny(gray, edges, lo, hi);

    // Close gaps + dilate to connect broken edges
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
    cv::dilate(edges, edges, kernel, cv::Point(-1, -1), 1);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    int mxLocal = px - x0;
    int myLocal = py - y0;

    QRect best;
    int bestArea = INT_MAX;
    for (const auto& cnt : contours) {
        cv::Rect br = cv::boundingRect(cnt);
        int w = br.width, h = br.height;
        if (w < 15 || h < 15) continue;
        if (w > wRegion * 0.92 && h > hRegion * 0.92) continue;
        double aspect = (double)qMax(w, h) / qMax(qMin(w, h), 1);
        if (aspect > 30) continue;
        if (!(br.x <= mxLocal && mxLocal <= br.x + br.width &&
              br.y <= myLocal && myLocal <= br.y + br.height)) continue;
        int area = w * h;
        if (area < bestArea) {
            bestArea = area;
            best = QRect(br.x, br.y, br.width, br.height);
        }
    }

    if (best.isNull()) return QRect();

    // Convert back to screen logical coordinates
    return QRect(
        qRound((x0 + best.x()) / dpr),
        qRound((y0 + best.y()) / dpr),
        qRound(best.width() / dpr),
        qRound(best.height() / dpr)
    );
}

// ===================== Color Copy =====================

void ScreenPreviewWin::copyColorValue() {
    qreal dpr = devicePixelRatioF();
    int px = qRound(m_mousePos.x() * dpr);
    int py = qRound(m_mousePos.y() * dpr);
    QImage img = m_srcPix.toImage();
    if (px >= 0 && px < img.width() && py >= 0 && py < img.height()) {
        QColor c = img.pixelColor(px, py);
        int r = c.red(), g = c.green(), b = c.blue();

        QString hexStr = QString("#%1%2%3")
            .arg(r, 2, 16, QChar('0'))
            .arg(g, 2, 16, QChar('0'))
            .arg(b, 2, 16, QChar('0'));
        hexStr = m_hexUpper ? hexStr.toUpper() : hexStr.toLower();
        if (m_hideHash) hexStr = hexStr.mid(1);

        QString rgbStr = QString("%1, %2, %3").arg(r).arg(g).arg(b);

        QString text;
        if (m_colorFmt == "HEX") {
            text = hexStr;
        } else if (m_colorFmt == "RGB+HEX") {
            text = rgbStr;  // top value is RGB
        } else if (m_colorFmt == "HEX+RGB") {
            text = hexStr;  // top value is HEX
        } else {
            text = rgbStr;  // RGB
        }
        QApplication::clipboard()->setText(text);
    }
}

// ===================== Capture Finalization =====================

void ScreenPreviewWin::onToolbarAction(CaptureAction action) {
    if (action == CaptureAction::CANCEL) {
        closeAll();
        return;
    }
    finishCapture(action);
}

void ScreenPreviewWin::finishCapture(CaptureAction action) {
    if (!m_selRect.isValid() || m_selRect.width() < 2 || m_selRect.height() < 2) {
        closeAll();
        return;
    }
    restoreSystemCursors();

    // Crop using DPR-aware scale (matching Python version)
    qreal scaleX = (width() > 0) ? (qreal)m_srcPix.width() / (qreal)width() : 1.0;
    qreal scaleY = (height() > 0) ? (qreal)m_srcPix.height() / (qreal)height() : 1.0;
    QRect cropRect(
        int(m_selRect.x() * scaleX),
        int(m_selRect.y() * scaleY),
        int(m_selRect.width() * scaleX),
        int(m_selRect.height() * scaleY)
    );
    QPixmap cropPix = m_srcPix.copy(cropRect);
    QImage cropped = cropPix.toImage();

    // Convert local rect to global screen coordinates (matching Python)
    QRect globalRect = m_selRect.translated(geometry().topLeft());

    emit captured(cropped, globalRect, action);
    closeAll();
}

void ScreenPreviewWin::showToolbar() {
    if (!m_hasSelection || m_selRect.isNull()) return;
    m_toolbar->reposition(m_selRect, rect());
    m_toolbar->show();
    m_toolbar->raise();
}

void ScreenPreviewWin::closeAll() {
    restoreSystemCursors();
    if (m_allWins) {
        for (auto* w : *m_allWins) {
            if (w && w->m_toolbar) w->m_toolbar->hide();
        }
        for (auto* w : *m_allWins) {
            if (w) {
                w->hide();
                w->close();
                // NOTE: deleteLater is called by startScreenCapture() cleanup loop
                //       to avoid double-delete (closeAll may be called from within
                //       finishCapture which itself is inside loop.exec()).
            }
        }
    } else {
        m_toolbar->hide();
        hide();
        close();
        // deleteLater handled by caller
    }
    emit cancelled();
}

// ===================== Entry Point =====================

CaptureResult startScreenCapture() {
    QApplication* app = qApp;
    if (!app) {
        qWarning() << "startScreenCapture requires QApplication";
        return CaptureResult();
    }

    QList<QScreen*> screens = app->screens();
    QVector<QPair<QRect, QPixmap>> screenData;
    for (QScreen* scr : screens) {
        QPixmap pix = scr->grabWindow(0);
        if (pix.isNull()) continue;
        screenData.append({scr->geometry(), pix});
    }
    if (screenData.isEmpty()) return CaptureResult();

    QVector<WinRectInfo> allWinRects = enumerateWindowRects();

    CaptureResult result;
    bool done = false;
    QEventLoop loop;
    QList<ScreenPreviewWin*> winList;

    for (int i = 0; i < screenData.size(); i++) {
        auto* w = new ScreenPreviewWin(screenData[i].second, screenData[i].first, allWinRects, i, &winList);
        winList.append(w);
        QObject::connect(w, &ScreenPreviewWin::captured, [&](const QImage& img, const QRect& r, CaptureAction a) {
            if (done) return;
            done = true;
            result.image = img;
            result.rect = r;
            result.action = a;
            loop.quit();
        });
        QObject::connect(w, &ScreenPreviewWin::cancelled, [&]() {
            if (done) return;
            done = true;
            loop.quit();
        });
    }

    for (auto* w : winList) {
        w->show();
        w->raise();
        w->activateWindow();
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)w->winId();
        if (hwnd) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (hwnd) SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
    }
    QApplication::processEvents();
    QApplication::processEvents();
#ifdef Q_OS_WIN
    // Re-assert after processEvents
    for (auto* w : winList) {
        HWND hwnd = (HWND)w->winId();
        if (hwnd) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (hwnd) SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#endif

    QTimer::singleShot(120000, &loop, &QEventLoop::quit);
    loop.exec();

    for (auto* w : winList) {
        w->close();
        w->deleteLater();
    }
    return result;
}

} // namespace snap

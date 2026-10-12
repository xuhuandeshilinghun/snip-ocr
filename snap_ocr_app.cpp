#include "snap_ocr_app.h"
#include "core/ui_theme.h"
#include "core/i18n.h"
#include "core/config_manager.h"
#include "core/resource_helper.h"
#include "core/img_res_loader.h"
#include "core/system_tray.h"
#include "core/hotkey_manager.h"
#include "core/delay_capture.h"
#include "core/ocr_worker.h"
#include "ui/ocr_result_dialog.h"
#include "ui/settings_dialog.h"
#include "ui/about_dialog.h"
#include "core/screen_capture.h"
#include "core/sticker_window.h"
#include "core/ocr_engine.h"
#include "core/updater.h"
#include "quirc.h"

#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QRectF>
#include <QTimer>
#include <QKeyEvent>
#include <QFileDialog>
#include <QClipboard>
#include <QInputDialog>
#include <QRandomGenerator>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QRect>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// ─────────────── DesktopToast ───────────────
// Simple bottom-right toast (no Q_OBJECT) defined inside cpp for self-containment.
class DesktopToast : public QWidget {
public:
    explicit DesktopToast(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                       Qt::Tool | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
    }

    void showToast(const QString& text, int duration = 0)
    {
        m_text = text;
        m_fixed = (duration <= 0);
        if (m_timer->isActive()) m_timer->stop();
        if (!m_fixed) m_timer->start(duration);
        updatePosition();
        update();
        show();
        raise();
    }

    void hideToast()
    {
        if (m_timer->isActive()) m_timer->stop();
        hide();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()), 12, 12);
        p.setClipPath(path);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 200));
        p.drawRect(rect());
        QFont f("Microsoft YaHei", 12);
        p.setFont(f);
        p.setPen(QColor(255, 255, 255));
        p.drawText(rect(), Qt::AlignCenter, m_text);
    }

private:
    void updatePosition()
    {
        QScreen* screen = QApplication::primaryScreen();
        if (!screen) return;
        QRect geo = screen->availableGeometry();
        QFont font("Microsoft YaHei", 12);
        QFontMetrics fm(font);
        int tw = fm.horizontalAdvance(m_text);
        int th = fm.ascent();
        int padX = 20, padY = 12;
        int w = tw + padX * 2;
        int h = th + padY * 2;
        int margin = 16;
        int x = geo.right() - w - margin;
        int y = geo.bottom() - h - margin;
        setGeometry(x, y, w, h);
    }

    QString m_text;
    QTimer* m_timer = nullptr;
    bool m_fixed = false;
};

// ─────────────── PathToast (delay-capture save-path popup) ───────────────
// Lightweight bottom-right toast for delay-capture save path with click-to-copy + close.
class PathToast : public QWidget {
    Q_OBJECT
public:
    explicit PathToast(const QString& filePath, int count, QWidget* parent = nullptr)
        : QWidget(parent), m_path(filePath), m_count(count), m_dark(UiTheme::is_dark_theme())
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
        QFont font("Microsoft YaHei", 10);
        QFontMetrics fm(font);
        QString label = I18n::t("delay.saved_to", QStringLiteral("\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98\xe5\x88\xb0: ")) + m_path;
        int textW = fm.horizontalAdvance(label);
        int btnW = 70;
        m_w = qMin(textW + btnW + 56, 800);
        m_h = 52;
        resize(m_w, m_h);
        positionBottomRight();
    }

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton) {
            QApplication::clipboard()->setText(m_path);
            m_copied = true;
            update();
            QTimer::singleShot(1200, this, [this]() { m_copied = false; update(); });
        }
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton) {
            int x = e->position().x();
            if (x >= m_w - 72) close();
        }
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()), 10, 10);
        p.setClipPath(path);
        p.setPen(Qt::NoPen);
        p.setBrush(m_dark ? QColor(30, 30, 30, 235) : QColor(245, 245, 245, 240));
        p.drawRect(rect());
        p.setBrush(QColor(0, 122, 204));
        p.drawRect(0, 0, 4, height());

        QFont font("Microsoft YaHei", 10);
        p.setFont(font);
        p.setPen(m_dark ? QColor(255, 255, 255) : QColor(51, 51, 51));
        if (m_copied) {
            QString label = I18n::t("msg.copied", QStringLiteral("\xe5\xb7\xb2\xe5\xa4\x8d\xe5\x88\xb6"));
            p.drawText(rect().adjusted(12, 0, -80, 0),
                       Qt::AlignVCenter | Qt::AlignLeft, label);
        } else {
            QString label;
            if (m_count > 1) {
                label = I18n::t("delay.loop_count",
                                QStringLiteral("\xe5\xbe\xaa\xe7\x8e\xaf\xe6\x88\xaa\xe5\x9b\xbe {n} \xe5\xbc\xa0\xef\xbc\x8c\xe4\xbf\x9d\xe5\xad\x98\xe5\x88\xb0: "))
                            .replace("{n}", QString::number(m_count)) + m_path;
            } else {
                label = I18n::t("delay.saved_to",
                                QStringLiteral("\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98\xe5\x88\xb0: ")) + m_path;
            }
            QFontMetrics fm(font);
            int avail = m_w - 80;
            QString elided = fm.elidedText(label, Qt::ElideMiddle, avail);
            p.drawText(rect().adjusted(12, 0, -80, 0),
                       Qt::AlignVCenter | Qt::AlignLeft, elided);
        }

        QRectF btnRect(m_w - 72, 8, 56, m_h - 16);
        QPainterPath btnPath;
        btnPath.addRoundedRect(btnRect, 6, 6);
        p.setBrush(m_dark ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 20));
        p.setPen(Qt::NoPen);
        p.drawPath(btnPath);
        p.setPen(QColor(180, 180, 180));
        p.setFont(QFont("Microsoft YaHei", 9));
        p.drawText(btnRect.toRect(), Qt::AlignCenter,
                   I18n::t("delay.close", QStringLiteral("\xe5\x85\xb3\xe9\x97\xad")));
    }

private:
    void positionBottomRight()
    {
        QScreen* screen = QApplication::primaryScreen();
        if (!screen) return;
        QRect geo = screen->availableGeometry();
        move(geo.right() - m_w - 16, geo.bottom() - m_h - 16);
    }

    QString m_path;
    int m_count;
    bool m_dark;
    bool m_copied = false;
    int m_w;
    int m_h;
};

// ─────────────── SnipOCRApp ───────────────

SnipOCRApp::SnipOCRApp(QWidget* parent)
    : QWidget(parent)
{
    m_desktopToast = new DesktopToast(this);
    m_delayCapture = new DelayCapture(this);
    connect(m_delayCapture, &DelayCapture::captured, this, &SnipOCRApp::onDelayCaptured);
    connect(m_delayCapture, &DelayCapture::cancelled, this, &SnipOCRApp::onDelayCancelled);

    m_lastTheme = ConfigManager::instance().get("theme", "dark").toString();
    m_lastLang = ConfigManager::instance().get("language", "zh_CN").toString();
    reloadQtTranslation(m_lastLang);  // Load Qt built-in translations for dialogs

    // init window: hidden, off-screen
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setFixedSize(0, 0);
    move(-10000, -10000);
    show();
    hide();

    // init tray
    m_tray = new TrayManager(this);
    connect(m_tray, &TrayManager::screenshotRequested, this, &SnipOCRApp::doScreenshot);
    connect(m_tray, &TrayManager::pinRequested, this, &SnipOCRApp::doPinScreenshot);
    connect(m_tray, &TrayManager::ocrRequested, this, &SnipOCRApp::doOcrScreenshot);
    connect(m_tray, &TrayManager::delayRequested, this, &SnipOCRApp::doDelayScreenshot);
    connect(m_tray, &TrayManager::settingsRequested, this, &SnipOCRApp::showSettings);
    connect(m_tray, &TrayManager::aboutRequested, this, &SnipOCRApp::showAbout);
    connect(m_tray, &TrayManager::updateCheckRequested, this, &SnipOCRApp::showCheckUpdate);
    connect(m_tray, &TrayManager::exitRequested, this, &SnipOCRApp::onExit);
    connect(m_tray, &TrayManager::groupSwitchRequested, this, &SnipOCRApp::switchGroup);
    connect(m_tray, &TrayManager::groupCreateRequested, this, &SnipOCRApp::showCreateGroupDialog);
    connect(m_tray, &TrayManager::clearHistoryRequested, this, &SnipOCRApp::onClearHistory);
    connect(m_tray, &TrayManager::hotkeyToggleRequested, this, &SnipOCRApp::onToggleHotkeys);
    connect(m_tray, &TrayManager::restartRequested, this, &SnipOCRApp::onRestart);
    m_tray->showIcon();

    ensureGroupFolders();
    restoreStickers();

    // Check if update just completed
    if (Updater::is_update_just_done()) {
        QString ver = Updater::get_current_version();
        QTimer::singleShot(500, this, [this, ver]() {
            if (m_desktopToast) {
                m_desktopToast->showToast(
                    I18n::t("updater.update_success", "更新成功") +
                    QStringLiteral(" V%1").arg(ver), 3000);
            }
        });
    }

    // delay-init hotkeys to ensure tray/Qt event loop is ready
    QTimer::singleShot(100, this, [this]() {
        m_hotkeyMgr = new GlobalHotkeyManager(this);
        connect(m_hotkeyMgr, &GlobalHotkeyManager::snapshotTriggered,
                this, &SnipOCRApp::doScreenshot);
        connect(m_hotkeyMgr, &GlobalHotkeyManager::pinTriggered,
                this, &SnipOCRApp::doPinScreenshot);
        connect(m_hotkeyMgr, &GlobalHotkeyManager::ocrTriggered,
                this, &SnipOCRApp::doOcrScreenshot);
        connect(m_hotkeyMgr, &GlobalHotkeyManager::delayTriggered,
                this, &SnipOCRApp::doDelayScreenshot);
        if (!m_hotkeyMgr->start()) {
            qWarning() << "[HOTKEY] Failed to start hotkey manager";
        }
        
        if (ConfigManager::instance().get("hotkey_disabled", false).toBool()) {
            m_hotkeyMgr->setEnabled(false);
        }
    });

    
    QTimer::singleShot(500, this, [this]() {
        Updater::check_and_show_update(this, true);
    });
}

SnipOCRApp::~SnipOCRApp()
{
    if (m_hotkeyMgr) m_hotkeyMgr->stop();
    for (OCRWorker* w : m_oldWorkers) {
        w->cancel();
        w->wait(2000);
    }
    if (m_ocrWorker) {
        m_ocrWorker->cancel();
        m_ocrWorker->wait(3000);
    }
}

// ─────────────── Screenshot flow ───────────────

void SnipOCRApp::doScreenshot()
{
    if (m_capturing) return;
    m_capturing = true;
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(false);
    // Remove WS_EX_TOPMOST from sticker windows directly via Win32
    sticker::StickerWindow::suspendTopmost(true);
    QList<HWND> stickerHwnds;
    for (auto* s : m_stickers) {
        HWND hwnd = reinterpret_cast<HWND>(s->winId());
        if (hwnd) {
            LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex & ~WS_EX_TOPMOST);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | 0x0020);
            stickerHwnds.append(hwnd);
        }
    }
    ::Sleep(50);
    snap::CaptureResult result = snap::startScreenCapture();
    // Restore WS_EX_TOPMOST
    sticker::StickerWindow::suspendTopmost(false);
    for (HWND hwnd : stickerHwnds) {
        LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex | WS_EX_TOPMOST);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(true);
    m_capturing = false;

    if (result.image.isNull() || result.action == snap::CaptureAction::CANCEL) return;
    handleCaptureResult(result);
}

void SnipOCRApp::doPinScreenshot()
{
    if (m_capturing) return;
    m_capturing = true;
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(false);
    // Remove WS_EX_TOPMOST from sticker windows directly via Win32
    sticker::StickerWindow::suspendTopmost(true);
    QList<HWND> stickerHwnds;
    for (auto* s : m_stickers) {
        HWND hwnd = reinterpret_cast<HWND>(s->winId());
        if (hwnd) {
            LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex & ~WS_EX_TOPMOST);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | 0x0020);
            stickerHwnds.append(hwnd);
        }
    }
    ::Sleep(50);
    snap::CaptureResult result = snap::startScreenCapture();
    // Restore WS_EX_TOPMOST
    sticker::StickerWindow::suspendTopmost(false);
    for (HWND hwnd : stickerHwnds) {
        LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex | WS_EX_TOPMOST);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(true);
    m_capturing = false;

    if (result.image.isNull() || result.action == snap::CaptureAction::CANCEL) return;
    
    QString historyPath = saveHistory(result.image);
    pinImage(result.image, result.rect, historyPath);
}

void SnipOCRApp::doOcrScreenshot()
{
    if (m_capturing) return;
    m_capturing = true;
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(false);
    // Remove WS_EX_TOPMOST from sticker windows directly via Win32
    sticker::StickerWindow::suspendTopmost(true);
    QList<HWND> stickerHwnds;
    for (auto* s : m_stickers) {
        HWND hwnd = reinterpret_cast<HWND>(s->winId());
        if (hwnd) {
            LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex & ~WS_EX_TOPMOST);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | 0x0020);
            stickerHwnds.append(hwnd);
        }
    }
    ::Sleep(50);
    snap::CaptureResult result = snap::startScreenCapture();
    // Restore WS_EX_TOPMOST
    sticker::StickerWindow::suspendTopmost(false);
    for (HWND hwnd : stickerHwnds) {
        LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex | WS_EX_TOPMOST);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(true);
    m_capturing = false;

    if (result.image.isNull() || result.action == snap::CaptureAction::CANCEL) return;
    saveHistory(result.image);
    ocrImage(result.image);
}

void SnipOCRApp::doDelayScreenshot()
{
    if (m_capturing || m_delayCapture->isActive()) return;
    int seconds = ConfigManager::instance().get("delay_seconds", 5).toInt();
    m_delayCapture->start(seconds);
    m_delayCount = 0;
}

void SnipOCRApp::onDelayCaptured()
{
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(false);
    try {
        QScreen* primary = QApplication::primaryScreen();
        if (!primary) return;
        QRect geo = primary->geometry();
        QPixmap pix = primary->grabWindow(0, geo.x(), geo.y(), geo.width(), geo.height());
        if (pix.isNull()) {
            if (m_desktopToast)
                m_desktopToast->showToast(I18n::t("delay.capture_failed",
                        QStringLiteral("\xe6\x88\xaa\xe5\xb1\x8f\xe5\xa4\xb1\xe8\xb4\xa5")), 2000);
            return;
        }
        QImage qimg = pix.toImage();
        QString historyPath = saveHistory(qimg);
        if (!historyPath.isEmpty()) {
            m_lastDelayPath = historyPath;
            m_delayCount++;
            bool loop = ConfigManager::instance().get("delay_loop", false).toBool();
            if (!loop) showSavePathToast(historyPath, 1);
        }
    } catch (...) {
        qWarning() << "[DELAY] Delay capture error";
    }
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(true);
    bool loop = ConfigManager::instance().get("delay_loop", false).toBool();
    if (loop) {
        int seconds = ConfigManager::instance().get("delay_seconds", 5).toInt();
        m_delayCapture->start(seconds);
    }
}

void SnipOCRApp::onDelayCancelled()
{
    // Close existing path toasts to avoid duplicates
    for (QWidget* w : QApplication::topLevelWidgets()) {
        auto* pt = qobject_cast<PathToast*>(w);
        if (pt) pt->close();
    }
    if (!m_lastDelayPath.isEmpty()) {
        showSavePathToast(m_lastDelayPath, m_delayCount);
    } else if (m_desktopToast) {
        m_desktopToast->showToast(I18n::t("delay.cancelled",
                QStringLiteral("\xe5\xbb\xb6\xe6\x97\xb6\xe6\x88\xaa\xe5\x9b\xbe\xe5\xb7\xb2\xe5\x8f\x96\xe6\xb6\x88")), 1500);
    }
}

void SnipOCRApp::handleCaptureResult(const snap::CaptureResult& result)
{
    const QImage& qimg = result.image;
    QString historyPath = saveHistory(qimg);
    switch (result.action) {
    case snap::CaptureAction::PIN:
        pinImage(qimg, result.rect, historyPath);
        break;
    case snap::CaptureAction::COPY:
        copyImage(qimg);
        break;
    case snap::CaptureAction::SAVE:
        saveImage(qimg);
        break;
    case snap::CaptureAction::OCR:
        ocrImage(qimg);
        break;
    case snap::CaptureAction::QR:
        qrImage(qimg);
        break;
    default:
        break;
    }
}

void SnipOCRApp::pinImage(const QImage& img, const QRect& rect, const QString& historyPath)
{
    QPixmap pix = QPixmap::fromImage(img);
    QString snipPath;
    if (!historyPath.isEmpty()) snipPath = copyToSnip(historyPath);

    // DPR: screenshot image is at physical pixels; rect is logical.
    // Set devicePixelRatio so StickerWindow renders sharp AND sizes correctly.
    qreal dpr = 1.0;
    if (rect.width() > 0 && rect.height() > 0) {
        dpr = (qreal)img.width() / (qreal)rect.width();
        if (dpr < 1.0) dpr = 1.0;
    }
    if (dpr > 1.01) pix.setDevicePixelRatio(dpr);

    // Position: pin at the exact screenshot region position (like Snipaste).
    int m = sticker::StickerWindow::SHADOW_MARGIN;
    QPoint pos;
    if (rect.isValid()) {
        pos = QPoint(rect.x() - m, rect.y() - m);
    } else {
        QPoint trayPos = getTrayIconTopRight();
        pos = QPoint(trayPos.x() - m, trayPos.y() - pix.height() + m);
    }

    auto* s = new sticker::StickerWindow(pix, nullptr, pos, snipPath, historyPath);
    s->setKeepStateOnClose(false);
    s->setAttribute(Qt::WA_DeleteOnClose);
    connect(s, &sticker::StickerWindow::closed, this, [this, s]() { removeSticker(s); });
    s->show();
    s->raise();
    m_stickers.append(s);

    if (!snipPath.isEmpty()) s->saveStickerState();
}

void SnipOCRApp::copyImage(const QImage& img)
{
    QApplication::clipboard()->setPixmap(QPixmap::fromImage(img));
    if (m_tray) m_tray->notify(I18n::t("msg.copied",
            QStringLiteral("\xe5\xb7\xb2\xe5\xa4\x8d\xe5\x88\xb6\xe5\x88\xb0\xe5\x89\xaa\xe8\xb4\xb4\xe6\x9d\xbf")));
}

void SnipOCRApp::saveImage(const QImage& img)
{
    QString path = QFileDialog::getSaveFileName(this,
            I18n::t("capture.save_title", QStringLiteral("\xe4\xbf\x9d\xe5\xad\x98\xe5\x9b\xbe\xe7\x89\x87")),
            QString(), "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    if (!path.isEmpty()) {
        img.save(path);
        if (m_tray) m_tray->notify(I18n::t("msg.saved_to",
                QStringLiteral("\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98\xe5\x88\xb0: ")) + path);
    }
}

void SnipOCRApp::ocrImage(const QImage& img)
{
    if (img.isNull()) return;
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat bgr(rgb.height(), rgb.width(), CV_8UC3);
    for (int y = 0; y < rgb.height(); y++) {
        const uint8_t* src = rgb.constScanLine(y);
        uint8_t* dst = bgr.ptr<uint8_t>(y);
        for (int x = 0; x < rgb.width(); x++) {
            dst[x * 3 + 0] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3 + 0];
        }
    }
    QVector<cv::Mat> images;
    images.append(bgr);
    startOcr(images);
}

void SnipOCRApp::qrImage(const QImage& img)
{
    if (img.isNull()) {
        if (m_tray) m_tray->notify(I18n::t("qr.no_code",
                QString::fromUtf8("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81")));
        return;
    }

    
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
    int w = gray.width();
    int h = gray.height();

    struct quirc *q = quirc_new();
    if (!q) {
        if (m_tray) m_tray->notify(I18n::t("qr.no_code",
                QString::fromUtf8("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81")));
        return;
    }

    if (quirc_resize(q, w, h) != 0) {
        quirc_destroy(q);
        if (m_tray) m_tray->notify(I18n::t("qr.no_code",
                QString::fromUtf8("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81")));
        return;
    }

    uint8_t *buf = quirc_begin(q, NULL, NULL);
    if (!buf) {
        quirc_destroy(q);
        if (m_tray) m_tray->notify(I18n::t("qr.no_code",
                QString::fromUtf8("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81")));
        return;
    }

    
    for (int y = 0; y < h; ++y) {
        memcpy(buf + y * w, gray.scanLine(y), w);
    }

    quirc_end(q);

    int count = quirc_count(q);
    QString resultText;
    for (int i = 0; i < count; ++i) {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract(q, i, &code);
        quirc_decode_error_t err = quirc_decode(&code, &data);
        if (err == QUIRC_SUCCESS) {
            
            QString text = QString::fromUtf8(reinterpret_cast<const char*>(data.payload), data.payload_len);
            if (text.isEmpty()) {
                text = QString::fromLatin1(reinterpret_cast<const char*>(data.payload), data.payload_len);
            }
            if (!resultText.isEmpty()) resultText += "\n";
            resultText += text;
        }
    }

    quirc_destroy(q);

    if (resultText.isEmpty()) {
        if (m_tray) m_tray->notify(I18n::t("qr.no_code",
                QString::fromUtf8("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81")));
        return;
    }

    
    QApplication::clipboard()->setText(resultText);
    if (!m_resultDlg || !m_resultDlg->isVisible()) {
        m_resultDlg = new OCRResultDialog(resultText, this);
        m_resultDlg->show();
    } else {
        m_resultDlg->updateText(resultText);
    }
    m_resultDlg->raise();
    m_resultDlg->activateWindow();
}

void SnipOCRApp::startOcr(const QVector<cv::Mat>& images)
{
    if (m_ocrWorker) {
        if (m_ocrWorker->isRunning()) {
            m_ocrWorker->cancel();
            m_ocrWorker->wait(5000);
        }
        m_oldWorkers.append(m_ocrWorker);
        m_ocrWorker = nullptr;
    }
    QString modelPath = resolveModelPath();
    m_ocrWorker = new OCRWorker(images, modelPath);
    connect(m_ocrWorker, &OCRWorker::finished, this, &SnipOCRApp::onOcrFinished);
    connect(m_ocrWorker, &OCRWorker::error, this, &SnipOCRApp::onOcrError);
    connect(m_ocrWorker, &OCRWorker::progress, this, &SnipOCRApp::onOcrProgress);
    m_ocrWorker->start();
    if (m_desktopToast)
        m_desktopToast->showToast(I18n::t("msg.ocr_processing",
                QStringLiteral("\xe6\xad\xa3\xe5\x9c\xa8OCR...")), 0);
}

void SnipOCRApp::onOcrFinished(const QString& text)
{
    cleanupOldWorkers();
    if (m_desktopToast) {
        if (!text.trimmed().isEmpty())
            m_desktopToast->showToast(I18n::t("msg.ocr_done",
                    QStringLiteral("OCR\xe5\xae\x8c\xe6\x88\x90")), 1500);
        else
            m_desktopToast->hideToast();
    }
    if (text.trimmed().isEmpty()) {
        if (m_tray) m_tray->notify(I18n::t("ocr.no_text",
                QStringLiteral("\xe6\x9c\xaa\xe8\xaf\x86\xe5\x88\xab\xe5\x88\xb0\xe6\x96\x87\xe5\xad\x97")));
        return;
    }
    QApplication::clipboard()->setText(text);
    if (!m_resultDlg || !m_resultDlg->isVisible()) {
        m_resultDlg = new OCRResultDialog(text, this);
        m_resultDlg->show();
    } else {
        m_resultDlg->updateText(text);
    }
    m_resultDlg->raise();
    m_resultDlg->activateWindow();
}

void SnipOCRApp::onOcrError(const QString& errMsg)
{
    cleanupOldWorkers();
    qWarning() << "[OCR] error:" << errMsg;
    if (m_desktopToast) m_desktopToast->hideToast();
    if (m_tray) {
        m_tray->notify(I18n::t("ocr.error",
                QStringLiteral("OCR Error: {error}")).replace("{error}", errMsg));
    }
}

void SnipOCRApp::onOcrProgress(const QString& msg)
{
    if (m_desktopToast) m_desktopToast->showToast(msg, 0);
}

void SnipOCRApp::cleanupOldWorkers()
{
    QList<OCRWorker*> alive;
    for (OCRWorker* w : m_oldWorkers) {
        if (w->isRunning()) alive.append(w);
        else w->deleteLater();
    }
    m_oldWorkers = alive;
}

// ─────────────── Group management ───────────────

QJsonArray SnipOCRApp::getGroups() const
{
    QJsonArray groups = ConfigManager::instance().get("groups").toJsonArray();
    if (groups.isEmpty()) {
        QJsonObject def;
        def["id"] = "default";
        def["name"] = QStringLiteral("default");
        groups.append(def);
        ConfigManager::instance().set("groups", groups);
    }
    return groups;
}

QString SnipOCRApp::getActiveGroupId() const
{
    QString active = ConfigManager::instance().get("active_group_id", "default").toString();
    QJsonArray groups = getGroups();
    QStringList ids;
    for (const QJsonValue& g : groups) ids << g.toObject().value("id").toString();
    if (!ids.contains(active)) active = ids.isEmpty() ? "default" : ids.first();
    return active;
}

int SnipOCRApp::getGroupCount(const QString& groupId) const
{
    QDir base = ResourceHelper::runtime_base_dir();
    QDir d = base.filePath("history/" + groupId);
    if (!d.exists()) return 0;
    QStringList files = d.entryList(QStringList() << "sp*.png", QDir::Files);
    return files.size();
}

void SnipOCRApp::ensureGroupFolders()
{
    QDir base = ResourceHelper::runtime_base_dir();
    QDir historyDir = base.filePath("history");
    historyDir.mkpath(".");
    QJsonArray groups = getGroups();
    for (const QJsonValue& g : groups) {
        QString gid = g.toObject().value("id").toString();
        QDir d = historyDir.filePath(gid);
        d.mkpath(".");
    }
    QDir snipDir = historyDir.filePath("snip");
    snipDir.mkpath(".");
}

void SnipOCRApp::createGroup(const QString& name, bool activate)
{
    // Generate random 6-char id (uppercase + digits)
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QJsonArray groups = getGroups();
    QSet<QString> existingIds;
    for (const QJsonValue& g : groups) existingIds << g.toObject().value("id").toString();
    QString gid;
    forever {
        gid.clear();
        for (int i = 0; i < 6; ++i) gid += chars.at(QRandomGenerator::global()->bounded(chars.size()));
        if (!existingIds.contains(gid)) break;
    }
    QJsonObject g;
    g["id"] = gid;
    g["name"] = name;
    groups.append(g);
    ConfigManager::instance().set("groups", groups);
    QDir base = ResourceHelper::runtime_base_dir();
    QDir historyDir = base.filePath("history");
    QDir d = historyDir.filePath(gid);
    d.mkpath(".");
    if (activate) {
        switchGroup(gid);
    } else {
        if (m_tray) {
            m_tray->refreshMenu();
            m_tray->notify(I18n::t("msg.group_created",
                    QStringLiteral("\xe5\xb7\xb2\xe5\x88\x9b\xe5\xbb\xba\xe5\x88\x86\xe7\xbb\x84\xef\xbc\x9a{name}"))
                        .replace("{name}", name));
        }
    }
}

QString SnipOCRApp::saveHistory(const QImage& img)
{
    if (img.isNull()) return QString();
    QDir base = ResourceHelper::runtime_base_dir();
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("yyyyMMdd");
    QString timeStr = now.toString("HHmmss");
    QString groupId = getActiveGroupId();
    QDir histDir = base.filePath("history/" + groupId);
    histDir.mkpath(".");
    int maxSeq = 0;
    QStringList files = histDir.entryList(QStringList() << "sp*.png", QDir::Files);
    for (const QString& fn : files) {
        QString stem = QFileInfo(fn).completeBaseName();
        QStringList parts = stem.split('_');
        if (parts.size() == 3) {
            bool ok = false;
            int n = parts[2].toInt(&ok);
            if (ok && n > maxSeq) maxSeq = n;
        }
    }
    int seq = maxSeq + 1;
    QString filename = QStringLiteral("sp%1_%2_%3.png")
                            .arg(dateStr).arg(timeStr).arg(seq, 3, 10, QChar('0'));
    QString fpath = histDir.filePath(filename);
    if (!img.save(fpath, "PNG")) return QString();
    return fpath;
}

QString SnipOCRApp::copyToSnip(const QString& originalPath)
{
    QFileInfo srcInfo(originalPath);
    if (!srcInfo.exists()) return QString();
    QFile src(originalPath);
    if (!src.open(QIODevice::ReadOnly)) return QString();
    QByteArray pngBytes = src.readAll();
    src.close();
    QDir base = ResourceHelper::runtime_base_dir();
    QDir snipDir = base.filePath("history/snip");
    snipDir.mkpath(".");
    QString sp1Name = ResourceHelper::to_sp1_name(srcInfo.fileName());
    QString dst = snipDir.filePath(sp1Name);
    if (!ResourceHelper::save_encrypted_png_bytes(pngBytes, dst)) return QString();
    QString rel = getActiveGroupId() + "/" + srcInfo.fileName();
    ResourceHelper::set_snip_entry(sp1Name, rel);
    return dst;
}

void SnipOCRApp::restoreStickers()
{
    QDir base = ResourceHelper::runtime_base_dir();
    QDir historyDir = base.filePath("history");
    QJsonObject index = ResourceHelper::load_snip_index();
    QJsonObject entries = index.value("snip_entries").toObject();
    QJsonObject states = index.value("sticker_states").toObject();
    QString activeGroup = getActiveGroupId();
    QString prefix = activeGroup + "/";
    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        QString snipName = it.key();
        QString origRel = it.value().toString();
        if (!origRel.startsWith(prefix)) continue;
        if (!states.contains(snipName)) continue;
        QString snipPath = historyDir.filePath("snip/" + snipName);
        if (!QFile::exists(snipPath)) continue;
        QString origPath = historyDir.filePath(origRel);
        
        // if (!QFile::exists(origPath)) origPath.clear();

        auto* s = sticker::StickerWindow::loadFromState(snipPath);
        if (!s) continue;

        QJsonObject st = states.value(snipName).toObject();
        qreal opacity = st.value("opacity").toDouble(1.0);
        bool hMirror = st.value("h_mirror").toBool(false);
        bool vMirror = st.value("v_mirror").toBool(false);

        s->setHistoryFilePath(snipPath);
        s->setOriginalFilePath(origPath);
        s->setOpacity(opacity);
        s->setHMirrored(hMirror);
        s->setVMirrored(vMirror);
        s->setKeepStateOnClose(true);
        s->setAttribute(Qt::WA_DeleteOnClose);
        connect(s, &sticker::StickerWindow::closed, this, [this, s]() { removeSticker(s); });
        s->show();
        m_stickers.append(s);
    }
}

void SnipOCRApp::switchGroup(const QString& groupId)
{
    ConfigManager::instance().set("active_group_id", groupId);
    QString name = groupId;
    for (const QJsonValue& g : getGroups()) {
        if (g.toObject().value("id").toString() == groupId) {
            name = g.toObject().value("name").toString(groupId);
            break;
        }
    }
    if (groupId == "default") name = I18n::t("tray.group_default_name", name);

    // Close all current stickers but keep their state.
    // Use a copy because s->close() emits closed() which calls
    // removeSticker() and modifies m_stickers during iteration.
    QList<sticker::StickerWindow*> oldStickers = m_stickers;
    for (sticker::StickerWindow* s : oldStickers) {
        s->saveStickerState();
        s->setKeepStateOnClose(true);
        s->close();
    }
    m_stickers.clear();

    // Re-create PathToast instance if any open, close them
    for (QWidget* w : QApplication::topLevelWidgets()) {
        auto* pt = qobject_cast<PathToast*>(w);
        if (pt) pt->close();
    }

    // Restore stickers for new active group
    restoreStickers();

    if (m_tray) {
        m_tray->notify(I18n::t("msg.group_switched",
                QStringLiteral("\xe5\xb7\xb2\xe5\x88\x87\xe6\x8d\xa2\xe5\x88\xb0\xe5\x88\x86\xe7\xbb\x84\xef\xbc\x9a{name}"))
                    .replace("{name}", name));
        m_tray->refreshMenu();
    }
}

void SnipOCRApp::showCreateGroupDialog()
{
    QInputDialog dlg(nullptr, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    dlg.setWindowTitle(I18n::t("tray.group_name_title", QStringLiteral("\xe6\x96\xb0\xe5\xbb\xba\xe5\x88\x86\xe7\xbb\x84")));
    dlg.setLabelText(I18n::t("tray.group_name_prompt", QStringLiteral("\xe8\xaf\xb7\xe8\xbe\x93\xe5\x85\xa5\xe5\x88\x86\xe7\xbb\x84\xe5\x90\x8d\xe7\xa7\xb0\xef\xbc\x9a")));
    dlg.setTextValue(I18n::t("tray.group_name_default", QStringLiteral("\xe6\x96\xb0\xe5\x88\x86\xe7\xbb\x84")));
    dlg.setTextEchoMode(QLineEdit::Normal);
    dlg.setOkButtonText(I18n::t("common.ok", QStringLiteral("确定")));
    dlg.setCancelButtonText(I18n::t("common.cancel", QStringLiteral("取消")));
    // Center on primary screen
    QScreen* scr = QApplication::primaryScreen();
    if (scr) {
        QRect geo = scr->availableGeometry();
        QSize sz = dlg.sizeHint();
        dlg.move(geo.center().x() - sz.width() / 2,
                 geo.center().y() - sz.height() / 2);
    }
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.textValue().trimmed();
        if (!name.isEmpty()) {
            createGroup(name, true);
        }
    }
}

void SnipOCRApp::removeSticker(sticker::StickerWindow* s)
{
    m_stickers.removeAll(s);
}

// ─────────────── Toast / tray-icon helpers ───────────────

void SnipOCRApp::showToast(const QString& text, int duration)
{
    if (m_desktopToast) m_desktopToast->showToast(text, duration);
}

void SnipOCRApp::hideToast()
{
    if (m_desktopToast) m_desktopToast->hideToast();
}

void SnipOCRApp::showSavePathToast(const QString& filePath, int count)
{
    auto* toast = new PathToast(filePath, count);
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->show();
    toast->raise();
    toast->activateWindow();
}

QPoint SnipOCRApp::getTrayIconTopRight() const
{
    QPoint pos = SnipOCRApp::findTrayNotificationArea();
    if (!pos.isNull()) return pos;
    if (m_tray) {
        QPoint tp = m_tray->getTrayPosition();
        if (!tp.isNull()) return QPoint(tp.x() + 16, tp.y() - 16);
    }
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        return QPoint(geo.right(), geo.bottom());
    }
    return QPoint();
}

QPoint SnipOCRApp::findTrayNotificationArea()
{
#ifdef Q_OS_WIN
    HWND hwndBar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hwndBar) return QPoint();
    HWND hwndNotify = FindWindowExW(hwndBar, nullptr, L"TrayNotifyWnd", nullptr);
    HWND target = hwndNotify ? hwndNotify : hwndBar;
    RECT r;
    if (!GetWindowRect(target, &r)) return QPoint();
    QPoint topRight(r.right, r.top);
    if (hwndNotify) {
        HWND hwndTool = FindWindowExW(hwndNotify, nullptr, L"ToolbarWindow32", nullptr);
        if (hwndTool) {
            RECT r2;
            if (GetWindowRect(hwndTool, &r2) && r2.right > r2.left && r2.bottom > r2.top) {
                topRight = QPoint(r2.right, r2.top);
            }
        }
    }
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        qreal dpr = screen->devicePixelRatio();
        if (dpr > 0) topRight = QPoint(int(topRight.x() / dpr), int(topRight.y() / dpr));
    }
    return topRight;
#else
    return QPoint();
#endif
}

QString SnipOCRApp::resolveModelPath() const
{
    QString modelPath = QDir(QCoreApplication::applicationDirPath()).filePath("model/MediaOCR-Base-v1.mtk-model");
    if (!QFileInfo::exists(modelPath)) {
        modelPath = "D:/Work/project/test/python/20260912/snap-text_C/model/MediaOCR-Base-v1.mtk-model";
    }
    return modelPath;
}

// ─────────────── Settings / about / update ───────────────

void SnipOCRApp::showSettings()
{
    if (m_settingsDlg && m_settingsDlg->isVisible()) {
        m_settingsDlg->raise();
        m_settingsDlg->activateWindow();
        return;
    }
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(false);
    m_settingsDlg = new SettingsDialog(this);
    m_settingsDlg->setAttribute(Qt::WA_DeleteOnClose);
    m_settingsDlg->setHotkeyManager(m_hotkeyMgr);
    connect(m_settingsDlg, &SettingsDialog::settingsChanged,
            this, &SnipOCRApp::onSettingsChanged);
    connect(m_settingsDlg, &QDialog::finished, this, [this]() {
        if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(true);
        m_settingsDlg = nullptr;
    });
    m_settingsDlg->raise();
    m_settingsDlg->activateWindow();
    m_settingsDlg->show();
}

void SnipOCRApp::showAbout()
{
    auto* dlg = new AboutDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->raise();
    dlg->activateWindow();
    dlg->show();
}

void SnipOCRApp::showCheckUpdate()
{
    Updater::check_and_show_update(this);
}

void SnipOCRApp::reloadQtTranslation(const QString& langCode) {
    // Remove old translation and load new one for Qt built-in dialogs
    qApp->removeTranslator(&m_qtTranslator);
    QString qmName = "qtbase_" + langCode;
    QString transDir;
    QDir searchDir = QCoreApplication::applicationDirPath();
    for (int i = 0; i < 6; ++i) {
        QString candidate = searchDir.absolutePath() + "/translations";
        if (QDir(candidate).exists()) { transDir = candidate; break; }
        if (!searchDir.cdUp()) break;
    }
    bool loaded = false;
    if (!transDir.isEmpty() && QFile::exists(transDir + "/" + qmName + ".qm")) {
        loaded = m_qtTranslator.load(qmName, transDir);
    }
    if (!loaded) {
        loaded = m_qtTranslator.load(qmName, QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    }
    if (loaded) {
        qApp->installTranslator(&m_qtTranslator);
    }
}

void SnipOCRApp::onSettingsChanged()
{
    QString newTheme = ConfigManager::instance().get("theme", "dark").toString();
    bool themeChanged = (newTheme != m_lastTheme);
    m_lastTheme = newTheme;

    QString newLang = ConfigManager::instance().get("language", "zh_CN").toString();
    bool langChanged = (newLang != m_lastLang);
    m_lastLang = newLang;
    I18n::set_language(newLang);
    if (langChanged) reloadQtTranslation(newLang);

    if (themeChanged) {
        // StickerWindow::applyTheme() is called per-sticker (guarded via qobject_cast)
        for (sticker::StickerWindow* s : m_stickers) {
            if (s) s->applyTheme();
        }
        if (m_tray) m_tray->refreshTheme();
    }
    if (langChanged) {
        for (sticker::StickerWindow* s : m_stickers) {
            if (s) s->refreshLanguage();
        }
        if (m_tray) m_tray->refreshMenu();
    }
    if (m_tray) m_tray->refreshTooltip();
    QTimer::singleShot(10, this, [this]() {
        if (m_hotkeyMgr) m_hotkeyMgr->reloadHotkeys();
        if (m_tray) m_tray->refreshMenu();
    });
}

// ─────────────── Exit / keyPressEvent ───────────────

void SnipOCRApp::onExit()
{
    if (m_hotkeyMgr) m_hotkeyMgr->stop();
    if (m_ocrWorker) {
        m_ocrWorker->cancel();
        m_ocrWorker->wait(3000);
    }
    for (OCRWorker* w : m_oldWorkers) {
        w->cancel();
        w->wait(2000);
    }
    if (m_tray) m_tray->cleanup();
    // Use a copy because s->close() emits closed() which calls
    // removeSticker() and modifies m_stickers during iteration.
    QList<sticker::StickerWindow*> stickers = m_stickers;
    for (sticker::StickerWindow* s : stickers) {
        s->setKeepStateOnClose(true);
        s->saveStickerState();
        s->close();
    }
    m_stickers.clear();
    QApplication::quit();
}

void SnipOCRApp::onToggleHotkeys(bool disabled)
{
    if (m_hotkeyMgr) m_hotkeyMgr->setEnabled(!disabled);
    if (m_desktopToast) {
        m_desktopToast->showToast(
            disabled ? I18n::t("msg.hotkeys_disabled", QStringLiteral("快捷键已禁用"))
                     : I18n::t("msg.hotkeys_enabled", QStringLiteral("快捷键已启用")),
            1500);
    }
}

void SnipOCRApp::onClearHistory()
{
    
    QList<sticker::StickerWindow*> stickers = m_stickers;
    for (sticker::StickerWindow* s : stickers) {
        s->setKeepStateOnClose(false);  
        s->close();
    }
    m_stickers.clear();

    
    ResourceHelper::clear_all_history();

    
    ensureGroupFolders();

    if (m_desktopToast) {
        m_desktopToast->showToast(I18n::t("msg.history_cleared", QStringLiteral("截屏历史已清空")), 2000);
    }
}

void SnipOCRApp::onRestart()
{
    
    QList<sticker::StickerWindow*> stickers = m_stickers;
    for (sticker::StickerWindow* s : stickers) {
        s->setKeepStateOnClose(true);
        s->saveStickerState();
        s->close();
    }
    m_stickers.clear();

    if (m_hotkeyMgr) m_hotkeyMgr->stop();
    if (m_tray) m_tray->cleanup();

    
#ifdef Q_OS_WIN
    QString exePath = QCoreApplication::applicationFilePath();
    QString workDir = QCoreApplication::applicationDirPath();
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpFile = (LPCWSTR)exePath.utf16();
    sei.lpParameters = L"--restart";
    sei.lpDirectory = (LPCWSTR)workDir.utf16();
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
#endif

    QApplication::quit();
}

void SnipOCRApp::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_delayCapture->isActive()) {
        m_delayCapture->cancel();
        return;
    }
    QWidget::keyPressEvent(e);
}

#include "snap_ocr_app.moc"

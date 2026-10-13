/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "delay_capture.h"
#include "i18n.h"
#include "ui_theme.h"

#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <QRectF>
#include <QRect>
#include <windows.h>

// VK_ESCAPE / VK_RBUTTON come from <windows.h>.

CountdownOverlay::CountdownOverlay(int seconds, QWidget* parent)
    : QWidget(parent)
    , m_seconds(seconds)
    , m_current(seconds)
    , m_dark(UiTheme::is_dark_theme())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool |
                    Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    resize(m_size, m_size);
    centerOnScreen();
}

void CountdownOverlay::centerOnScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->geometry();
    int x = geo.x() + (geo.width() - m_size) / 2;
    int y = geo.y() + (geo.height() - m_size) / 2;
    move(x, y);
}

void CountdownOverlay::updateCountdown(int remaining)
{
    m_current = remaining;
    update();
}

void CountdownOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Rounded clip
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 24, 24);
    p.setClipPath(path);

    // Background
    QColor bg = m_dark ? QColor(20, 20, 20, 200) : QColor(40, 40, 40, 200);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRect(rect());

    // Left accent bar
    p.setBrush(QColor(0, 122, 204));
    p.drawRect(0, 0, 4, height());

    // Big number
    QFont numFont("Microsoft YaHei", 64, QFont::Bold);
    p.setFont(numFont);
    p.setPen(QColor(255, 255, 255));
    p.drawText(rect(), Qt::AlignCenter, QString::number(m_current));

    // Hint text
    QFont hintFont("Microsoft YaHei", 9);
    p.setFont(hintFont);
    p.setPen(QColor(180, 180, 180));
    QString hint = I18n::t("delay.hint", "ESC {cancel}")
                       .replace("{cancel}", I18n::t("delay.cancel", "Cancel"));
    p.drawText(QRect(0, height() - 30, width(), 24), Qt::AlignCenter, hint);
}

DelayCapture::DelayCapture(QObject* parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &DelayCapture::onTick);

    m_poll = new QTimer(this);
    m_poll->setInterval(50);
    connect(m_poll, &QTimer::timeout, this, &DelayCapture::pollKeys);
}

DelayCapture::~DelayCapture()
{
    if (m_overlay) {
        m_overlay->hide();
        m_overlay->deleteLater();
        m_overlay = nullptr;
    }
}

void DelayCapture::start(int seconds)
{
    if (m_active) return;
    m_active = true;
    m_remaining = seconds;
    m_overlay = new CountdownOverlay(seconds);
    m_overlay->updateCountdown(seconds);
    m_overlay->show();
    m_timer->start(1000);
    m_poll->start();
}

void DelayCapture::cancel()
{
    if (!m_active) return;
    m_timer->stop();
    m_poll->stop();
    m_active = false;
    if (m_overlay) {
        m_overlay->hide();
        m_overlay->deleteLater();
        m_overlay = nullptr;
    }
    emit cancelled();
}

void DelayCapture::pollKeys()
{
    if (!m_active) return;
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        cancel();
        return;
    }
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        cancel();
        return;
    }
}

void DelayCapture::onTick()
{
    m_remaining--;
    if (m_remaining <= 0) {
        m_timer->stop();
        m_poll->stop();
        m_active = false;
        if (m_overlay) {
            m_overlay->hide();
            m_overlay->deleteLater();
            m_overlay = nullptr;
        }
        emit captured();
        return;
    }
    if (m_overlay)
        m_overlay->updateCountdown(m_remaining);
}

/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QWidget>
#include <QObject>
#include <QTimer>

// Countdown overlay widget: centered, translucent, topmost, click-through.
class CountdownOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CountdownOverlay(int seconds, QWidget* parent = nullptr);
    void updateCountdown(int remaining);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    void centerOnScreen();
    int m_seconds;
    int m_current;
    bool m_dark;
    int m_size = 200;
};

// Delay capture manager: countdown timer + ESC/right-click cancel polling.
// Faithfully ports Python core/delay_capture.py.
class DelayCapture : public QObject {
    Q_OBJECT
public:
    explicit DelayCapture(QObject* parent = nullptr);
    ~DelayCapture() override;

    bool isActive() const { return m_active; }
    void start(int seconds);
    void cancel();

signals:
    void captured();
    void cancelled();

private:
    void onTick();
    void pollKeys();

    QTimer* m_timer = nullptr;
    QTimer* m_poll = nullptr;
    CountdownOverlay* m_overlay = nullptr;
    int m_remaining = 0;
    bool m_active = false;
};

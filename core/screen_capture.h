/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QPoint>
#include <QVector>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QToolButton>
#include <QFrame>
#include <QIcon>
#include <QHash>
#include <QPair>
#include <QByteArray>

namespace snap {

enum class CaptureAction {
    OCR = 0,
    COPY = 1,
    SAVE = 2,
    CANCEL = 3,
    PIN = 4,
    QR = 5
};

struct CaptureResult {
    QImage image;
    QRect rect;
    CaptureAction action = CaptureAction::CANCEL;
};

struct WinRectInfo {
    void* hwnd;
    QRect rect;
};

class CaptureToolbar : public QFrame {
    Q_OBJECT
public:
    explicit CaptureToolbar(QWidget* parent = nullptr);
    void reposition(const QRect& selRect, const QRect& screenGeo);
    void applyStyle();

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* e) override { setCursor(Qt::ArrowCursor); QWidget::enterEvent(e); }
    void leaveEvent(QEvent* e) override { QWidget::leaveEvent(e); }

signals:
    void actionTriggered(snap::CaptureAction action);

private:
    QHBoxLayout* m_layout;
    QHash<QString, QToolButton*> m_buttons;
    QHash<QString, QIcon> m_iconCache;

    QColor m_bgColor;
    QColor m_borderColor;
    QColor m_hoverColor;
    QColor m_textColor;
    bool m_dark = true;

    static QHash<QString, QString> iconSvgs();
    static QVector<QPair<QString, QString>> buttonDefs();
    QIcon renderSvgIcon(const QString& svgStr, const QColor& color, int size = 20);
    void refreshIcons(const QColor& color);
    void buildUi();
};

class ScreenPreviewWin : public QWidget {
    Q_OBJECT
public:
    ScreenPreviewWin(const QPixmap& srcPix, const QRect& screenGeo,
                     const QVector<WinRectInfo>& winRects, int screenIndex = 0,
                     QList<ScreenPreviewWin*>* allWins = nullptr);
    ~ScreenPreviewWin() override;

signals:
    void captured(const QImage& image, const QRect& rect, snap::CaptureAction action);
    void cancelled();

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* e) override { setCursor(Qt::ArrowCursor); QWidget::enterEvent(e); }
    void leaveEvent(QEvent* e) override { QWidget::leaveEvent(e); }
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    enum Handle { None = -1, TopLeft = 0, Top, TopRight, Right,
                  BottomRight, Bottom, BottomLeft, Left, Move };

    QPixmap m_srcPix;
    QImage m_srcImage;
    QRect m_screenGeo;
    int m_screenIndex;
    QList<ScreenPreviewWin*>* m_allWins;

    QPoint m_startPt;
    QPoint m_endPt;
    QRect m_selRect;
    bool m_isDragging = false;
    bool m_hasSelection = false;
    bool m_dragMoved = false;
    Handle m_resizeHandle = None;
    QPoint m_moveStart;
    QRect m_moveOrig;
    bool m_movingSelection = false;

    QVector<WinRectInfo> m_winRects;
    QRect m_snapRect;
    QPoint m_lastSnapPos = QPoint(-9999, -9999);
    QString m_snapMode = "window";

    int m_magSize = 150;
    int m_magZoom = 8;
    bool m_magVisible = true;
    QPoint m_mousePos;

    QString m_colorFmt = "RGB";
    bool m_hideHash = false;
    bool m_hexUpper = false;

    CaptureToolbar* m_toolbar;

    void loadMagnifierConfig();
    Handle hitTest(const QPoint& pos) const;
    void updateCursor(const QPoint& pos);
    QRect findSnapRect(const QPoint& pos);
    // Snipaste-style: snap selection edges to nearby window edges while resizing/moving
    void snapSelectionToEdges(QRect& r, Handle handle);
    QRect edgeSnap(const QPoint& pos);
    void drawMagnifier(QPainter& p);
    void copyColorValue();
    void onToolbarAction(snap::CaptureAction action);
    void finishCapture(snap::CaptureAction action);
    void showToolbar();
    void closeAll();
    void forceForeground();
    static void replaceSystemCursorsWithCross();
    static void restoreSystemCursors();
};

CaptureResult startScreenCapture();
QVector<WinRectInfo> enumerateWindowRects();

} // namespace snap

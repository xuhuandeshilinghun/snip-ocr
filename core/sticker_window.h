/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QColor>
#include <QFont>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
class QLineEdit;
class QTimer;
class QFrame;
class QVBoxLayout;
class QHBoxLayout;
QT_END_NAMESPACE

namespace sticker {


enum class Tool {
    None,
    Rect,
    Ellipse,
    Arrow,
    Pen,
    Highlighter,
    Mosaic,
    Text,
    Eraser
};


enum class ArrowStyle { Line, Simple, Taper, Hollow };


struct Annotation {
    Tool tool = Tool::None;
    QColor color;
    int width = 3;
    
    QPoint start;
    QPoint end;
    
    QVector<QPoint> points;
    
    QString text;
    QFont font;
    QPoint pos;
    
    int blockSize = 10;
    int mosaicOpacity = 6;   
    bool mosaicFill = false;   
    QString mosaicMode = "mosaic"; // "mosaic" / "blur"
    
    bool fill = false;
    
    QString hlMode = "sketch"; // "sketch" / "box"
    
    ArrowStyle arrowStyle = ArrowStyle::Taper;
    
    bool selected = false;
};

class ColorPanel;
class FontPanel;
class PenPanel;
class ShapeTypePanel;
class ArrowTypePanel;
class MosaicPanel;
class HighlighterTypePanel;
class EraserTypePanel;
class StickerToolbar;

class StickerWindow : public QWidget {
    Q_OBJECT
public:
    explicit StickerWindow(const QPixmap& pixmap, QWidget* parent = nullptr,
                          const QPoint& pos = QPoint(),
                          const QString& historyFilePath = QString(),
                          const QString& originalFilePath = QString(),
                          qreal scale = 1.0);
    ~StickerWindow() override;

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

    void applyTheme();
    void refreshLanguage();

    
    bool saveState(const QString& path);
    static StickerWindow* loadFromState(const QString& path, QWidget* parent = nullptr);
    static QString persistenceDir();
    static QStringList listPersistedStickers();
    static void suspendTopmost(bool suspend) { s_topmostSuspended = suspend; }

    
    void rotateImage(bool clockwise);
    void flipImage(bool horizontal);
    void zoomTo(qreal scale);
    void toggleSmooth();
    void toggleShadow(bool checked);
    void toggleToolbar(bool checked);
    QPixmap getRenderedPixmap() const;
    QPixmap getOriginalRenderedPixmap() const;
    void copyOriginalToClipboard();
    void saveOriginalToFile();
    void pasteFromClipboard();
    void replaceWithFile();
    void viewInFolder();
    void destroySticker();
    void moveToGroup(const QString& groupId);
    void createGroupAndMove();
    void showToast(const QString& text, int duration = 1500);
    void hideToast();
    bool saveSnipFile();
    void saveStickerState();
    void removeStickerState();
    static QIcon getCheckIcon();

    // Setters for restoration
    void setHistoryFilePath(const QString& path) { m_historyFilePath = path; }
    void setOriginalFilePath(const QString& path) { m_originalFilePath = path; }
    void setKeepStateOnClose(bool keep) { m_keepStateOnClose = keep; }
    void setOpacity(qreal opacity) { m_opacity = opacity; setWindowOpacity(opacity); }
    void setHMirrored(bool m) { m_hMirrored = m; }
    void setVMirrored(bool m) { m_vMirrored = m; }
    qreal scale() const { return m_scale; }
    qreal opacity() const { return m_opacity; }
    bool hMirrored() const { return m_hMirrored; }
    bool vMirrored() const { return m_vMirrored; }

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void adjustToolWidth(int delta);
    void contextMenuEvent(QContextMenuEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void changeEvent(QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void moveEvent(QMoveEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    
    QPoint toImagePos(const QPoint& windowPos) const;
    QPoint toWindowPos(const QPoint& imagePos) const;

    
    void drawAnnotation(QPainter& p, const Annotation& ann);
    void drawCurrentAnnotation(QPainter& p);
    void drawArrow(QPainter& p, const QPoint& start, const QPoint& end,
                   const QColor& color, int width, ArrowStyle style);
    void drawMosaic(QPainter& p, const QRect& rect, int blockSize, int opacity = 6);
    void drawBlur(QPainter& p, const QRect& rect, int blockSize, int opacity = 6);
    void bakeAnnotations();

    
    void rebuildStaticCache();
    void markCacheDirty();
    QPixmap m_staticCache;
    bool m_cacheDirty = true;

    
    void drawMagnifier(QPainter& p);
    QPoint m_mousePos;
    int m_magSize = 150;
    int m_magZoom = 4;
    bool m_magVisible = true;
    QString m_colorFmt = QStringLiteral("RGB");
    bool m_hideHash = false;
    bool m_hexUpper = false;

    
    void applyWin11DwmAttributes();

    
    void buildToolbar();
    void updateToolbar();
    void repositionToolbar();
    void repositionSubPanels();
    void closeAllPanels();
    void syncWidthLabel();

    
    void onToolChanged(const QString& toolKey);
    void onAction(const QString& actionKey);
    void setActiveTool(Tool tool);
    void undo();
    void redo();
    void clearAnnotations();
    void saveToFile();
    void copyToClipboard();
    void pinToDesktop();

    
    void showUnifiedPanel(const QString& toolKey);
    void hideUnifiedPanel();
    void onColorPicked(const QColor& c);
    void onShapeSelected(Tool tool, bool fill);
    void onArrowSelected(ArrowStyle style);
    void onHlSelected(const QString& mode);
    void onEraserSelected(const QString& mode);
    void onMosaicFill(bool fill);

    
    void eraseAtPos(const QPoint& imgPos);
    double distToLine(const QPoint& p, const QPoint& a, const QPoint& b) const;
    // text annotation inline edit
    QRect textBbox(const Annotation& ann) const;
    QRect textHandleRect(const Annotation& ann) const;
    bool hitTextHandle(const Annotation& ann, const QPoint& pos) const;
    void startInlineEdit(const QPoint& imgPos, int existingIdx = -1);
    void finishInlineEdit();
    void cancelInlineEdit();

    // advanced erase + hit test
    void eraseBox(const QRect& rect);
    QList<Annotation> splitArrow(const Annotation& ann, const QPoint& pos, int r) const;
    static QList<QVector<QPoint>> splitPathAtCircle(const QVector<QPoint>& points, const QPoint& center, int radius);
    bool annIntersectsRect(const Annotation& ann, const QRect& rect) const;
    static bool lineRectIntersect(const QPoint& p1, const QPoint& p2, const QRect& rect);
    static bool segmentsIntersect(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4);
    QString hitArrowEndpoint(const Annotation& ann, const QPoint& pos, int threshold = 12) const;
    bool hitTest(const Annotation& ann, const QPoint& pos) const;
    static QList<QPair<QPoint, QPoint>> rectEdges(const QRect& rect);

    QLineEdit* m_textEditor = nullptr;
    int m_textEditorIdx = -1;
    QPoint m_textEditorPos;


    void pushAnnotation(const Annotation& ann);
    void finishAnnotation();
    void refreshUndoRedo();

    
    void applyScale(qreal factor);
    void setTopmost(bool topmost);

    
    QPixmap m_pixmap;
    QPixmap m_originalPixmap;
    int m_imgW = 0;
    int m_imgH = 0;
    qreal m_scale = 1.0;
    bool m_hMirrored = false;
    bool m_vMirrored = false;
    int m_origImgW = 0;
    int m_origImgH = 0;
    qreal m_opacity = 1.0;
    bool m_smoothScaling = true;
    QString m_historyFilePath;
    QString m_originalFilePath;

public:
    static constexpr int SHADOW_MARGIN = 10;
    static bool s_topmostSuspended;
private:

    bool m_selected = false;
    bool m_showToolbar = false;

    
    QVector<Annotation> m_annotations;
    QVector<Annotation> m_redoStack;
    Tool m_currentTool = Tool::None;
    bool m_drawing = false;
    QPoint m_drawStart;
    QPoint m_drawCurrent;
    QPoint m_lastErasePos;
    QVector<QPoint> m_freehandPoints;

    
    bool m_dragging = false;
    QPoint m_dragOffset;

    
    bool m_draggingText = false;
    int m_draggingTextIdx = -1;
    QPoint m_textDragOffset;

    
    bool m_resizingText = false;
    int m_resizingTextIdx = -1;
    qreal m_resizeStartSize = 16;
    qreal m_resizeStartDist = 0;

    
    bool m_draggingArrow = false;
    int m_draggingArrowIdx = -1;
    QString m_draggingArrowEndpoint; // "start" / "end"

    
    QColor m_annColor = QColor(255, 0, 0);
    QFont m_annFont = QFont("Microsoft YaHei", 16);
    int m_penWidth = 3;
    int m_highlighterWidth = 15;
    int m_shapeWidth = 4;
    int m_arrowWidth = 4;
    int m_eraserWidth = 20;
    bool m_shapeFill = false;
    Tool m_shapeTool = Tool::Rect; 
    ArrowStyle m_arrowStyle = ArrowStyle::Taper;
    QString m_hlMode = "sketch";
    int m_mosaicBlock = 10;
    int m_mosaicOpacity = 6;   
    QString m_mosaicMode = "mosaic";
    bool m_mosaicFill = false;
    QString m_eraserMode = "free";

    
    StickerToolbar* m_toolbar = nullptr;
    ColorPanel* m_colorPanel = nullptr;
    FontPanel* m_fontPanel = nullptr;
    PenPanel* m_penPanel = nullptr;
    ShapeTypePanel* m_shapePanel = nullptr;
    ArrowTypePanel* m_arrowPanel = nullptr;
    MosaicPanel* m_mosaicPanel = nullptr;
    HighlighterTypePanel* m_hlPanel = nullptr;
    EraserTypePanel* m_eraserPanel = nullptr;

    
    QString m_toastText;
    QTimer* m_toastTimer = nullptr;
    bool m_shadowEnabled = true;
    bool m_keepStateOnClose = false;
};

} // namespace sticker

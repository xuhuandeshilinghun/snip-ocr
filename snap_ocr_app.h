#pragma once
#include <QWidget>
#include <QString>
#include <QPoint>
#include <QRect>
#include <QList>
#include <QPixmap>
#include <QImage>
#include <QTranslator>

class TrayManager;
class GlobalHotkeyManager;
class DelayCapture;
class OCRWorker;
class OCRResultDialog;
class StickerWindow;
class DesktopToast;
class SettingsDialog;
class QTimer;

namespace cv { class Mat; }
namespace sticker { class StickerWindow; }
namespace snap { struct CaptureResult; }

// Main application: hidden window for signal relay and child window parenting.
// Faithfully ports Python main.py SnipOCRApp class.
class SnipOCRApp : public QWidget {
    Q_OBJECT
public:
    explicit SnipOCRApp(QWidget* parent = nullptr);
    ~SnipOCRApp() override;

protected:
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void doScreenshot();
    void doPinScreenshot();
    void doOcrScreenshot();
    void doDelayScreenshot();
    void onDelayCaptured();
    void onDelayCancelled();
    void onOcrFinished(const QString& text);
    void onOcrError(const QString& errMsg);
    void onOcrProgress(const QString& msg);
    void onExit();
    void onClearHistory();
    void onToggleHotkeys(bool disabled);
    void onRestart();
    void onSettingsChanged();
    void reloadQtTranslation(const QString& langCode);
    void showSettings();
    void showAbout();
    void showCheckUpdate();
    void switchGroup(const QString& groupId);
    void showCreateGroupDialog();
    void removeSticker(sticker::StickerWindow* s);

public:
    void ocrImage(const QImage& img);
    void qrImage(const QImage& img);

private:
    // Screenshot flow
    void handleCaptureResult(const snap::CaptureResult& result);
    void pinImage(const QImage& img, const QRect& rect, const QString& historyPath);
    void copyImage(const QImage& img);
    void saveImage(const QImage& img);
    void startOcr(const QVector<cv::Mat>& images);
    void cleanupOldWorkers();

    // Group management
    QJsonArray getGroups() const;
    QString getActiveGroupId() const;
    int getGroupCount(const QString& groupId) const;
    void ensureGroupFolders();
    void createGroup(const QString& name, bool activate);
    QString saveHistory(const QImage& img);
    QString copyToSnip(const QString& originalPath);
    void restoreStickers();

    // Helpers
    QPoint getTrayIconTopRight() const;
    static QPoint findTrayNotificationArea();
    QString resolveModelPath() const;

    // Desktop toast
    void showToast(const QString& text, int duration = 0);
    void hideToast();
    void showSavePathToast(const QString& filePath, int count = 0);

    // Members
    QList<sticker::StickerWindow*> m_stickers;
    OCRWorker* m_ocrWorker = nullptr;
    QList<OCRWorker*> m_oldWorkers;
    GlobalHotkeyManager* m_hotkeyMgr = nullptr;
    TrayManager* m_tray = nullptr;
    DesktopToast* m_desktopToast = nullptr;
    DelayCapture* m_delayCapture = nullptr;
    OCRResultDialog* m_resultDlg = nullptr;
    SettingsDialog* m_settingsDlg = nullptr;
    bool m_capturing = false;
    QString m_lastDelayPath;
    int m_delayCount = 0;
    QString m_lastTheme;
    QString m_lastLang;
    QTranslator m_qtTranslator;  // Qt built-in translations (QColorDialog, etc.)
};

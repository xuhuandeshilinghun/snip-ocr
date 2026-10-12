#pragma once
#include <QDialog>
#include "core/updater.h"

class QProgressBar;
class QTextEdit;
class QPushButton;
class QLabel;

class UpdateDialog : public QDialog {
    Q_OBJECT
public:
    explicit UpdateDialog(const UpdateInfo& info, bool autoDownload = true, QWidget* parent = nullptr);

private slots:
    void onDownloadProgress(int pct, const QString& text);
    void onDownloadDone(bool ok, const QString& pathOrError);

private:
    void buildUi();
    void applyTheme();
    void centerOnScreen();
    void appendLog(const QString& msg);
    void setProgress(int value, const QString& fmt = QString());
    void startDownload();

    UpdateInfo m_info;
    bool m_autoDownload = true;
    bool m_skipped = false;
    QProgressBar* m_progressBar = nullptr;
    QTextEdit* m_logEdit = nullptr;
    QPushButton* m_btnCancel = nullptr;
    DownloadThread* m_dlThread = nullptr;
    int m_lastLogPct = -1;
};

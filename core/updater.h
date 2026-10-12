#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QThread>

// Update info parsed from remote version.json
struct UpdateInfo {
    QString latestVer;       // "1.0.0" (without V/v prefix)
    QString updateLog;       // changelog text
    bool forceUpdate = false;
    // full_package fields
    QString pkgUrl;
    QString pkgSha256;
    qint64 pkgSize = 0;
    // diff_package fields (optional)
    bool hasDiff = false;
    QString diffBaseVersion;  // base version required for diff (e.g. "1.0.0")
    QString diffUrl;
    QString diffSha256;
    qint64 diffSize = 0;
    int diffChangedFiles = 0;
    // runtime: which package to use
    bool useDiff = false;
    // internal: path to downloaded zip
    QString downloadedPkg;
    QString updateType;  // "full" or "diff"
};

// Background thread: probe version URLs, compare versions
class CheckUpdateThread : public QThread {
    Q_OBJECT
public:
    explicit CheckUpdateThread(const QStringList& urls, QObject* parent = nullptr);
    void run() override;
signals:
    // Emits UpdateInfo with non-empty latestVer if update available, empty if not
    void resultReady(const UpdateInfo& info);
private:
    QStringList m_urls;
};

// Background thread: download update package with SHA256 verification
class DownloadThread : public QThread {
    Q_OBJECT
public:
    explicit DownloadThread(const UpdateInfo& info, QObject* parent = nullptr);
    void run() override;
signals:
    void progressUpdated(int pct, const QString& text);
    void downloadDone(bool ok, const QString& pathOrError);
private:
    UpdateInfo m_info;
};

namespace Updater {
    // Version helpers
    QString get_current_version();
    QString get_build_date();  
    bool compare_version(const QString& local, const QString& remote);

    // SHA256 helpers
    QString calc_sha256(const QString& filePath);
    bool verify_sha256(const QString& filePath, const QString& expected);

    // Path safety
    bool is_safe_path(const QString& base, const QString& target);

    // Updater process
    QString find_updater_exe();
    bool run_updater_process(const UpdateInfo& info);
    void write_version_file(const QString& newVer);
    bool is_update_just_done();

    // Entry point: async check, show dialog or toast
    void check_and_show_update(QWidget* parent, bool silent = false);
}

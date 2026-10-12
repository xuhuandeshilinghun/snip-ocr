#include "core/updater.h"
#include "core/i18n.h"
#include "core/ui_theme.h"
#include "core/region_router.h"
#include "core/sys_env_helper.h"
#include "core/config_manager.h"
#include "ui/update_dialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QPointer>
#include <QScreen>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QSet>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ─── Helpers ──────────────────────────────────────────

static QByteArray syncHttpGet(const QString& urlStr, int timeoutMs = 3000) {
    QNetworkAccessManager nam;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SnipOCR-Updater/1.0");

    QNetworkReply* reply = nam.get(request);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        reply->abort();
        loop.quit();
    });

    timer.start(timeoutMs);
    loop.exec();
    timer.stop();

    QByteArray result;
    if (reply->error() == QNetworkReply::NoError) {
        result = reply->readAll();
    }
    reply->deleteLater();
    return result;
}

static QString get_app_dir() {
    return QCoreApplication::applicationDirPath();
}

static QList<int> parseVersionParts(const QString& ver) {
    QString clean = ver;
    if (clean.startsWith('V') || clean.startsWith('v')) {
        clean = clean.mid(1);
    }
    auto parts = clean.split('.');
    QList<int> result;
    for (int i = 0; i < 3; ++i) {
        result.append(i < parts.size() ? parts[i].toInt() : 0);
    }
    return result;
}

// ─── Version helpers ──────────────────────────────────

namespace Updater {

QString get_current_version() {
    QString versionFile = get_app_dir() + "/.version.json";
    QFile f(versionFile);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll();
        f.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QString ver = doc.object().value("latest_ver").toString();
            if (!ver.isEmpty()) {
                if (ver.startsWith('V') || ver.startsWith('v'))
                    ver = ver.mid(1);
                return ver;
            }
        }
    }
    return QStringLiteral("1.0.0");
}

QString get_build_date() {
    QString versionFile = get_app_dir() + "/.version.json";
    QFile f(versionFile);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll();
        f.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            qint64 ts = doc.object().value("updated_at").toVariant().toLongLong();
            if (ts > 0) {
                return QDateTime::fromSecsSinceEpoch(ts).toString("yyyy.MM.dd");
            }
        }
    }
    return QDateTime::currentDateTime().toString("yyyy.MM.dd");
}

bool compare_version(const QString& local, const QString& remote) {
    auto lp = parseVersionParts(local);
    auto rp = parseVersionParts(remote);
    for (int i = 0; i < 3; ++i) {
        if (rp[i] > lp[i]) return true;
        if (rp[i] < lp[i]) return false;
    }
    return false;
}

// ─── SHA256 helpers ───────────────────────────────────

QString calc_sha256(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const qint64 chunkSize = 8192;
    while (!f.atEnd()) {
        QByteArray chunk = f.read(chunkSize);
        if (chunk.isEmpty()) break;
        hash.addData(chunk);
    }
    f.close();
    return QString::fromLatin1(hash.result().toHex()).toLower();
}

bool verify_sha256(const QString& filePath, const QString& expected) {
    if (expected.isEmpty()) return true;
    if (!QFile::exists(filePath)) return true;
    return calc_sha256(filePath) == expected.toLower();
}

// ─── Path safety ──────────────────────────────────────

bool is_safe_path(const QString& base, const QString& target) {
    QDir baseDir(base);
    QDir targetDir(target);
    QString basePath = baseDir.absolutePath();
    QString targetPath = targetDir.absolutePath();
    return targetPath.startsWith(basePath, Qt::CaseInsensitive);
}

// ─── Updater process ──────────────────────────────────

QString find_updater_exe() {
    QString appDir = get_app_dir();
    QStringList candidates = {
        appDir + "/updater.exe",
        appDir + "/SnipOCR-Updater.exe",
        appDir + "/updater/updater.exe",
        appDir + "/../updater/updater.exe",
        appDir + "/../../updater/updater.exe",
    };
    for (const QString& c : candidates) {
        QFileInfo fi(c);
        if (fi.exists() && fi.isExecutable()) {
            return fi.absoluteFilePath();
        }
    }
    return QString();
}

bool run_updater_process(const UpdateInfo& info) {
    QString appDir = get_app_dir();
    QString infoFile = appDir + "/_update_info.json";
    QString resultFile = appDir + "/_update_result.json";

    QFile::remove(resultFile);

    QJsonObject infoObj;
    infoObj["pkg_path"] = info.downloadedPkg;
    infoObj["app_dir"] = QDir(appDir).absolutePath();
    infoObj["new_ver"] = info.latestVer;
    infoObj["result_file"] = resultFile;
    infoObj["update_log"] = info.updateLog;
    infoObj["update_type"] = info.updateType;

    {
        QFile f(infoFile);
        if (!f.open(QIODevice::WriteOnly)) return false;
        f.write(QJsonDocument(infoObj).toJson(QJsonDocument::Compact));
        f.close();
    }

    QString updaterExe = find_updater_exe();
    if (updaterExe.isEmpty()) return false;

#ifdef Q_OS_WIN
    QString cmdLine = QString("\"%1\" install --info-json \"%2\"")
                          .arg(updaterExe).arg(infoFile);
    std::wstring cmdLineW = cmdLine.toStdWString();
    std::wstring appDirW = appDir.toStdWString();

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(
        nullptr,
        &cmdLineW[0],
        nullptr, nullptr, FALSE,
        0x00000008 | 0x08000000 | 0x00000200,  // DETACHED_PROCESS | CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP
        nullptr,
        appDirW.c_str(),
        &si, &pi
    );

    if (!ok) return false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    return false;
#endif
}

void write_version_file(const QString& newVer) {
    QString versionFile = get_app_dir() + "/.version.json";

    QString ver = newVer;
    if (!ver.startsWith('V') && !ver.startsWith('v')) {
        ver = "V" + ver;
    }

    QJsonObject obj;
    obj["latest_ver"] = ver;
    obj["updated_at"] = QDateTime::currentDateTime().toSecsSinceEpoch();

    QFile f(versionFile);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        f.close();
    }
}

bool is_update_just_done() {
    QString flagFile = get_app_dir() + "/_update_done.flag";
    if (QFile::exists(flagFile)) {
        QFile::remove(flagFile);
        return true;
    }
    return false;
}

// ─── Simple toast widget for "no update" notification ─

class UpdateToast : public QWidget {
public:
    static void showMessage(const QString& text, int duration = 2000) {
        auto* toast = new UpdateToast(text);
        toast->setAttribute(Qt::WA_DeleteOnClose);
        toast->showToast(duration);
    }

    explicit UpdateToast(const QString& text)
        : QWidget(nullptr), m_text(text)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);

        QFont font("Microsoft YaHei", 11);
        QFontMetrics fm(font);
        int textW = fm.horizontalAdvance(m_text);
        m_w = textW + 48;
        m_h = 44;
        resize(m_w, m_h);

        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect geo = screen->availableGeometry();
            move(geo.right() - m_w - 16, geo.bottom() - m_h - 16);
        }
    }

    void showToast(int duration) {
        show();
        if (duration > 0) {
            QTimer::singleShot(duration, this, &QWidget::close);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()), 10, 10);
        p.setClipPath(path);
        p.setPen(Qt::NoPen);
        bool dark = UiTheme::is_dark_theme();
        p.setBrush(dark ? QColor(30, 30, 30, 220) : QColor(245, 245, 245, 230));
        p.drawRect(rect());
        p.setBrush(QColor(0, 122, 204));
        p.drawRect(0, 0, 4, m_h);
        QFont font("Microsoft YaHei", 11);
        p.setFont(font);
        p.setPen(dark ? QColor(255, 255, 255) : QColor(51, 51, 51));
        p.drawText(rect().adjusted(12, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, m_text);
    }

private:
    QString m_text;
    int m_w = 0;
    int m_h = 0;
};

// ─── Entry point ──────────────────────────────────────

void check_and_show_update(QWidget* parent, bool silent) {
    QStringList urls = RegionRouter::get_version_urls();
    auto* thread = new CheckUpdateThread(urls);

    QPointer<QWidget> parentPtr(parent);

    QObject::connect(thread, &CheckUpdateThread::resultReady, parentPtr,
        [parentPtr, silent](const UpdateInfo& info) {
            if (info.latestVer.isEmpty()) {
                
                if (!silent) {
                    UpdateToast::showMessage(I18n::t("updater.mb_text_latest_version",
                        QStringLiteral("已是最新版本。")));
                }
                return;
            }
            
            QString skippedVer = ConfigManager::instance().get("skipped_version", "").toString();
            if (silent && !skippedVer.isEmpty() && skippedVer == info.latestVer) {
                
                return;
            }
            auto* dlg = new UpdateDialog(info, !silent, parentPtr);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->exec();
        });

    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

} // namespace Updater

// ─── CheckUpdateThread ────────────────────────────────

CheckUpdateThread::CheckUpdateThread(const QStringList& urls, QObject* parent)
    : QThread(parent), m_urls(urls) {}

void CheckUpdateThread::run() {
    bool isCn = SysEnvHelper::is_china_locale();

    QStringList giteeUrls, githubUrls;
    for (const QString& url : m_urls) {
        if (url.contains("gitee.com")) {
            giteeUrls.append(url);
        } else if (url.contains("github.com") || url.contains("raw.githubusercontent.com")) {
            githubUrls.append(url);
        }
    }

    QStringList ordered;
    if (isCn && !giteeUrls.isEmpty()) {
        ordered = giteeUrls + githubUrls;
    } else if (!isCn && !githubUrls.isEmpty()) {
        ordered = githubUrls + giteeUrls;
    } else {
        ordered = m_urls;
    }

    QJsonObject data;
    for (const QString& url : ordered) {
        QByteArray body = syncHttpGet(url, 3000);
        if (body.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError) continue;
        if (!doc.isObject()) continue;

        data = doc.object();
        break;
    }

    if (data.isEmpty()) {
        emit resultReady(UpdateInfo{});
        return;
    }

    QString remoteVer = data.value("latest_ver").toString(data.value("version").toString());
    if (remoteVer.startsWith('V') || remoteVer.startsWith('v')) {
        remoteVer = remoteVer.mid(1);
    }

    QString localVer = Updater::get_current_version();

    if (!Updater::compare_version(localVer, remoteVer)) {
        emit resultReady(UpdateInfo{});
        return;
    }

    UpdateInfo info;
    info.latestVer = remoteVer;
    info.updateLog = data.value("update_log").toString(data.value("changelog").toString());
    info.forceUpdate = data.value("force_update").toBool(false);

    QJsonObject fullPkg = data.value("full_package").toObject();
    info.pkgUrl = RegionRouter::build_download_url(remoteVer, fullPkg.value("filename").toString());
    info.pkgSha256 = fullPkg.value("sha256").toString();
    info.pkgSize = fullPkg.value("size").toVariant().toLongLong();

    // Parse diff_package if present
    QJsonObject diffPkg = data.value("diff_package").toObject();
    if (!diffPkg.isEmpty()) {
        info.hasDiff = true;
        info.diffBaseVersion = diffPkg.value("base_version").toString();
        info.diffUrl = RegionRouter::build_download_url(remoteVer, diffPkg.value("filename").toString());
        info.diffSha256 = diffPkg.value("sha256").toString();
        info.diffSize = diffPkg.value("size").toVariant().toLongLong();
        info.diffChangedFiles = diffPkg.value("changed_files").toInt(0);

        // Use diff if local version exactly matches base version
        if (!info.diffBaseVersion.isEmpty()) {
            QString localVer = Updater::get_current_version();
            if (localVer == info.diffBaseVersion) {
                info.useDiff = true;
            }
        }
    }

    // Set update type
    info.updateType = info.useDiff ? "diff" : "full";

    emit resultReady(info);
}

// ─── DownloadThread ───────────────────────────────────

DownloadThread::DownloadThread(const UpdateInfo& info, QObject* parent)
    : QThread(parent), m_info(info) {}

void DownloadThread::run() {
    QString url = m_info.useDiff ? m_info.diffUrl : m_info.pkgUrl;
    QString expectedSha256 = m_info.useDiff ? m_info.diffSha256 : m_info.pkgSha256;

    if (url.isEmpty()) {
        emit downloadDone(false, I18n::t("updater.dl_url_empty",
            QStringLiteral("下载地址为空")));
        return;
    }

    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        url = "https://" + url;
    }

    QString pkgPath = QDir::tempPath() + QString("/SnipOCR_update_%1.zip")
                          .arg(QCoreApplication::applicationPid());

    emit progressUpdated(5, I18n::t("updater.dl_connecting_server",
        QStringLiteral("正在连接更新服务器...")));

    const int maxRetries = 3;
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        QNetworkAccessManager nam;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        QUrl dlUrl(url);
        QNetworkRequest request(dlUrl);
        request.setHeader(QNetworkRequest::UserAgentHeader,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36");

        QNetworkReply* reply = nam.get(request);

        QFile file(pkgPath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadDone(false, QStringLiteral("Cannot create temp file"));
            reply->deleteLater();
            return;
        }

        qint64 total = 0;
        qint64 downloaded = 0;
        qint64 pkgSize = m_info.useDiff ? m_info.diffSize : m_info.pkgSize;
        bool gotError = false;
        QString errorMsg;

        QObject::connect(reply, &QNetworkReply::metaDataChanged, [&]() {
            auto cl = reply->header(QNetworkRequest::ContentLengthHeader);
            if (cl.isValid()) {
                total = cl.toLongLong();
            }
        });

        QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
            QByteArray data = reply->readAll();
            file.write(data);
            downloaded += data.size();

            int pct;
            if (total > 0) {
                pct = (int)(downloaded * 85 / total);
            } else if (pkgSize > 0) {
                pct = (int)(downloaded * 85 / pkgSize);
            } else {
                pct = qMin(85, (int)(downloaded / 1048576 * 5));
            }
            pct = qMax(5, qMin(85, pct));

            double mbDone = downloaded / 1048576.0;
            double mbTotal = (total > 0 ? total : pkgSize) / 1048576.0;
            QString text;
            if (mbTotal > 0) {
                text = I18n::t("updater.dl_downloading_pct",
                    QStringLiteral("下载中... {pct}% ({done}/{total}MB)"))
                    .replace("{pct}", QString::number(pct))
                    .replace("{done}", QString::number(mbDone, 'f', 1))
                    .replace("{total}", QString::number(mbTotal, 'f', 1));
            } else {
                text = I18n::t("updater.dl_downloading_pct",
                    QStringLiteral("下载中... {pct}%"))
                    .replace("{pct}", QString::number(pct));
            }
            emit progressUpdated(pct, text);
        });

        QObject::connect(reply, &QNetworkReply::errorOccurred, [&](QNetworkReply::NetworkError) {
            gotError = true;
            errorMsg = reply->errorString();
        });

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, [&]() {
            gotError = true;
            errorMsg = QStringLiteral("Timeout");
            reply->abort();
            loop.quit();
        });

        timer.start(120000);
        loop.exec();
        timer.stop();

        bool success = (!gotError && reply->error() == QNetworkReply::NoError);
        file.close();
        reply->deleteLater();

        if (success) {
            if (!expectedSha256.isEmpty()) {
                emit progressUpdated(90, I18n::t("updater.dl_verifying_integrity",
                    QStringLiteral("正在校验文件完整性...")));
                if (!Updater::verify_sha256(pkgPath, expectedSha256)) {
                    QFile::remove(pkgPath);
                    emit downloadDone(false, I18n::t("updater.dl_sha256_failed",
                        QStringLiteral("SHA256校验失败，文件可能被篡改")));
                    return;
                }
            }

            emit progressUpdated(92, I18n::t("updater.dl_done_preparing",
                QStringLiteral("下载完成，准备安装...")));
            emit downloadDone(true, pkgPath);
            return;
        }

        QFile::remove(pkgPath);
        if (attempt < maxRetries - 1) {
            int wait = 1 << attempt;
            QThread::sleep(wait);
        } else {
            emit downloadDone(false, I18n::t("updater.retry_failed",
                QStringLiteral("{error}（已重试{count}次）"))
                .replace("{error}", errorMsg)
                .replace("{count}", QString::number(maxRetries)));
            return;
        }
    }
}

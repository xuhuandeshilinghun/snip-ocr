#include "ui/update_dialog.h"
#include "core/updater.h"
#include "core/i18n.h"
#include "core/ui_theme.h"
#include "core/config_manager.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QDateTime>
#include <QScrollBar>

#ifdef Q_OS_WIN
#include <cstdlib>
#endif

UpdateDialog::UpdateDialog(const UpdateInfo& info, bool autoDownload, QWidget* parent)
    : QDialog(parent), m_info(info), m_autoDownload(autoDownload)
{
    buildUi();
    applyTheme();
    centerOnScreen();

    if (m_autoDownload) {
        QTimer::singleShot(150, this, &UpdateDialog::startDownload);
    }
}

void UpdateDialog::buildUi()
{
    QString newVer = m_info.latestVer;
    if (newVer.startsWith('V') || newVer.startsWith('v'))
        newVer = newVer.mid(1);

    QString changelog = m_info.updateLog;
    bool force = m_info.forceUpdate;

    setWindowTitle(I18n::t("updater.window_title",
        QStringLiteral("发现新版本 V%1")).arg(newVer));
    setMinimumSize(560, 460);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 14);
    layout->setSpacing(8);

    // Version comparison row
    auto* verLayout = new QHBoxLayout();
    QString curVer = Updater::get_current_version();

    auto* lblCur = new QLabel(I18n::t("updater.current_version",
        QStringLiteral("当前版本：%1")).arg(curVer), this);
    lblCur->setFont(QFont("Microsoft YaHei", 10));

    auto* lblArrow = new QLabel(QStringLiteral("  →  "), this);
    lblArrow->setFont(QFont("Microsoft YaHei", 10));

    auto* lblNew = new QLabel(I18n::t("updater.latest_version",
        QStringLiteral("最新版本：V%1")).arg(newVer), this);
    QFont newFont("Microsoft YaHei", 10, QFont::Bold);
    lblNew->setFont(newFont);
    lblNew->setStyleSheet("color: #4CAF50;");

    verLayout->addWidget(lblCur);
    verLayout->addWidget(lblArrow);
    verLayout->addWidget(lblNew);
    verLayout->addStretch();
    layout->addLayout(verLayout);

    // Update type badge (diff / full)
    if (m_info.useDiff) {
        auto* diffBadge = new QLabel(
            I18n::t("updater.diff_badge",
                QStringLiteral("差分更新  (%1 个文件)")).arg(m_info.diffChangedFiles), this);
        diffBadge->setStyleSheet(
            "background: #FF9800; color: white; padding: 2px 8px; "
            "border-radius: 4px; font-size: 11px; font-weight: bold;");
        diffBadge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        layout->addWidget(diffBadge);
    }

    // Separator
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Changelog title
    auto* logTitle = new QLabel(I18n::t("updater.log_label",
        QStringLiteral("更新日志：")), this);
    logTitle->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    layout->addWidget(logTitle);

    // Changelog scroll area
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QScrollArea::NoFrame);
    auto* logFrame = new QFrame();
    auto* logLayout = new QVBoxLayout(logFrame);
    auto* changelogLabel = new QLabel(
        changelog.isEmpty() ? I18n::t("updater.changelog_empty",
            QStringLiteral("暂无更新日志")) : changelog, logFrame);
    changelogLabel->setWordWrap(true);
    changelogLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    changelogLabel->setFont(QFont("Microsoft YaHei", 10));
    logLayout->addWidget(changelogLabel);
    scroll->setWidget(logFrame);
    layout->addWidget(scroll, 1);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(I18n::t("updater.preparing",
        QStringLiteral("准备中...")));
    layout->addWidget(m_progressBar);

    // Log detail label
    auto* logDetail = new QLabel(I18n::t("updater.log_label_detail",
        QStringLiteral("更新日志详情：")), this);
    logDetail->setFont(QFont("Microsoft YaHei", 9));
    layout->addWidget(logDetail);

    // Log text edit
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(110);
    m_logEdit->setFont(QFont("Consolas", 9));
    layout->addWidget(m_logEdit);

    // Button row
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    if (!m_autoDownload) {
        
        if (!force) {
            
            auto* btnSkip = new QPushButton(I18n::t("updater.skip_version",
                QStringLiteral("跳过此版本")), this);
            btnSkip->setMinimumWidth(100);
            connect(btnSkip, &QPushButton::clicked, this, [this]() {
                ConfigManager::instance().set("skipped_version", m_info.latestVer);
                m_skipped = true;
                reject();
            });
            btnRow->addWidget(btnSkip);

            m_btnCancel = new QPushButton(I18n::t("common.later",
                QStringLiteral("以后再说")), this);
            m_btnCancel->setMinimumWidth(100);
            connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
            btnRow->addWidget(m_btnCancel);
        }

        
        auto* btnUpdate = new QPushButton(I18n::t("updater.update_now",
            QStringLiteral("立即更新")), this);
        btnUpdate->setMinimumWidth(100);
        btnUpdate->setStyleSheet("background: #4CAF50; color: white;");
        connect(btnUpdate, &QPushButton::clicked, this, [this, btnUpdate]() {
            btnUpdate->setVisible(false);
            if (m_btnCancel) m_btnCancel->setVisible(false);
            startDownload();
        });
        btnRow->addWidget(btnUpdate);
    } else {
        
        m_btnCancel = new QPushButton(I18n::t("common.later",
            QStringLiteral("稍后再说")), this);
        m_btnCancel->setMinimumWidth(100);
        connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        btnRow->addWidget(m_btnCancel);
    }
    layout->addLayout(btnRow);
}

void UpdateDialog::applyTheme()
{
    bool dark = UiTheme::is_dark_theme();
    QString bg = dark ? "#1e1e1e" : "#f5f5f5";
    QString text = dark ? "#ffffff" : "#333333";
    QString border = dark ? "#3d3d3d" : "#cccccc";
    QString logBg = dark ? "#2a2a2a" : "#1e1e1e";
    QString logText = "#d4d4d4";
    QString accent = dark ? "#007acc" : "#0078d4";
    QString accentHover = dark ? "#1a8cff" : "#106ebe";
    QString progressBg = dark ? "#2a2a2a" : "#e0e0e0";

    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; background: transparent; }"
        "QFrame { color: %3; }"
        "QScrollArea { background: transparent; border: 1px solid %3; border-radius: 4px; }"
        "QProgressBar {"
        "  background: %4; color: %2;"
        "  border: 1px solid %3; border-radius: 4px; text-align: center;"
        "}"
        "QProgressBar::chunk { background: %5; border-radius: 3px; }"
        "QTextEdit {"
        "  background: %6; color: %7;"
        "  border: 1px solid %3; border-radius: 4px; padding: 4px;"
        "}"
        "QPushButton {"
        "  background: %5; color: #ffffff;"
        "  border: none; border-radius: 4px;"
        "  padding: 6px 16px; font-size: 13px;"
        "}"
        "QPushButton:hover { background: %8; }"
    ).arg(bg, text, border, progressBg, accent, logBg, logText, accentHover));
}

void UpdateDialog::centerOnScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->availableGeometry();
    int x = geo.x() + (geo.width() - width()) / 2;
    int y = geo.y() + (geo.height() - height()) / 2;
    move(x, y);
}

void UpdateDialog::appendLog(const QString& msg)
{
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->append(QStringLiteral("[%1] %2").arg(ts, msg));
    auto* sb = m_logEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void UpdateDialog::setProgress(int value, const QString& fmt)
{
    m_progressBar->setValue(value);
    if (!fmt.isEmpty()) {
        m_progressBar->setFormat(fmt);
    }
    if (value != m_lastLogPct) {
        m_lastLogPct = value;
        appendLog(fmt.isEmpty() ? QStringLiteral("%1%").arg(value) : fmt);
    }
}

void UpdateDialog::startDownload()
{
    appendLog(QStringLiteral("============================================"));
    appendLog(I18n::t("updater.log_start_update", QStringLiteral("开始更新")));
    appendLog(I18n::t("updater.log_local_version",
        QStringLiteral("本地版本：V%1")).arg(Updater::get_current_version()));

    QString targetVer = m_info.latestVer;
    if (!targetVer.startsWith('V') && !targetVer.startsWith('v'))
        targetVer = "V" + targetVer;
    appendLog(I18n::t("updater.log_target_version",
        QStringLiteral("目标版本：%1")).arg(targetVer));

    setProgress(0, I18n::t("updater.dl_connecting_server",
        QStringLiteral("正在连接更新服务器...")));

    m_dlThread = new DownloadThread(m_info, this);
    connect(m_dlThread, &DownloadThread::progressUpdated,
            this, &UpdateDialog::onDownloadProgress);
    connect(m_dlThread, &DownloadThread::downloadDone,
            this, &UpdateDialog::onDownloadDone);
    connect(m_dlThread, &QThread::finished,
            m_dlThread, &QObject::deleteLater);
    m_dlThread->start();
}

void UpdateDialog::onDownloadProgress(int pct, const QString& text)
{
    setProgress(pct, text);
}

void UpdateDialog::onDownloadDone(bool ok, const QString& pathOrError)
{
    if (!ok) {
        setProgress(100, I18n::t("updater.progress_download_failed",
            QStringLiteral("✗ 下载失败")));
        appendLog(QStringLiteral("[✗] %1").arg(pathOrError));
        m_btnCancel->setText(I18n::t("common.close", QStringLiteral("关闭")));
        m_btnCancel->setVisible(true);
        return;
    }

    m_info.downloadedPkg = pathOrError;
    setProgress(95, I18n::t("updater.progress_preparing_install",
        QStringLiteral("准备安装...")));
    appendLog(I18n::t("updater.log_starting_updater",
        QStringLiteral("正在启动安装程序...")));

    QString updaterExe = Updater::find_updater_exe();
    if (updaterExe.isEmpty()) {
        setProgress(100, I18n::t("updater.progress_download_failed",
            QStringLiteral("✗ 安装失败")));
        appendLog(I18n::t("updater.err_updater_not_found",
            QStringLiteral("未找到 updater.exe，无法自动安装。")));
        appendLog(I18n::t("updater.log_click_restart",
            QStringLiteral("请手动下载并替换程序文件。")));
        m_btnCancel->setText(I18n::t("common.close", QStringLiteral("关闭")));
        m_btnCancel->setVisible(true);
        return;
    }

    bool launched = Updater::run_updater_process(m_info);
    if (launched) {
        appendLog(I18n::t("updater.log_updater_started",
            QStringLiteral("安装程序已启动，主程序即将退出...")));
        setProgress(98, I18n::t("updater.log_main_exit",
            QStringLiteral("主程序退出中...")));
        QTimer::singleShot(2000, []() {
#ifdef Q_OS_WIN
            std::exit(0);
#else
            QCoreApplication::quit();
#endif
        });
    } else {
        setProgress(100, I18n::t("updater.progress_download_failed",
            QStringLiteral("✗ 启动安装失败")));
        appendLog(I18n::t("updater.err_updater_not_found",
            QStringLiteral("启动 updater.exe 失败。")));
        m_btnCancel->setText(I18n::t("common.close", QStringLiteral("关闭")));
        m_btnCancel->setVisible(true);
    }
}

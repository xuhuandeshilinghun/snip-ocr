#include "about_dialog.h"
#include "core/i18n.h"
#include "core/ui_theme.h"
#include "core/img_res_loader.h"
#include "core/region_router.h"
#include "core/updater.h"
#include "license_dialog.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QFile>
#include <QDesktopServices>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::t("about.title", QStringLiteral("\xe5\x85\xb3\xe4\xba\x8e SnipOCR")));
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setFixedSize(420, 420);

    buildUi();
    applyTheme();
    centerOnScreen();
}

QLabel* AboutDialog::makeLinkLabel(const QString& text, const QString& url)
{
    auto* lbl = new QLabel(
        QStringLiteral("<a href=\"%1\" style=\"color: #0078d4;\">%2</a>").arg(url).arg(text),
        this);
    lbl->setOpenExternalLinks(true);
    lbl->setFont(QFont("Microsoft YaHei", 10));
    return lbl;
}

void AboutDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(10);

    
    auto* top = new QHBoxLayout();
    top->setSpacing(12);

    auto* iconLabel = new QLabel(this);
    QPixmap icoPm = ImgResLoader::get_img_pixmap("logo.ico");
    if (!icoPm.isNull()) {
        iconLabel->setPixmap(icoPm.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setFixedSize(64, 64);

    auto* nameCol = new QVBoxLayout();
    nameCol->setSpacing(4);

    auto* nameLabel = new QLabel(I18n::t("about.app_name", QStringLiteral("SnipOCR")), this);
    QFont nameFont("Microsoft YaHei", 18, QFont::Bold);
    nameLabel->setFont(nameFont);

    auto* verLabel = new QLabel(
        QStringLiteral("%1 %2 (%3)")
            .arg(I18n::t("about.version", QStringLiteral("\xe7\x89\x88\xe6\x9c\xac")))
            .arg(Updater::get_current_version())
            .arg(Updater::get_build_date()), this);
    verLabel->setFont(QFont("Microsoft YaHei", 10));

    auto* descLabel = new QLabel(
        I18n::t("about.desc", QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe \xc2\xb7 OCR \xc2\xb7 \xe8\xb4\xb4\xe5\x9b\xbe\xe5\xb7\xa5\xe5\x85\xb7")), this);
    descLabel->setFont(QFont("Microsoft YaHei", 9));

    nameCol->addWidget(nameLabel);
    nameCol->addWidget(verLabel);
    nameCol->addWidget(descLabel);

    top->addWidget(iconLabel);
    top->addLayout(nameCol);
    top->addStretch();
    layout->addLayout(top);

    
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    
    auto* supportTitle = new QLabel(I18n::t("about.support", QStringLiteral("\xe6\x94\xaf\xe6\x8c\x81")), this);
    supportTitle->setFont(QFont("Microsoft YaHei", 11, QFont::Bold));
    layout->addWidget(supportTitle);

    
    QString issuesUrl = RegionRouter::get_issues_url();
    layout->addWidget(makeLinkLabel(
        I18n::t("about.official_site", QStringLiteral("\xe5\xae\x98\xe7\xbd\x91\xe9\xa6\x96\xe9\xa1\xb5")),
        QStringLiteral("https://github.com/xuhuandeshilinghun/")));
    layout->addWidget(makeLinkLabel(
        I18n::t("about.manual", QStringLiteral("\xe4\xbd\xbf\xe7\x94\xa8\xe6\x89\x8b\xe5\x86\x8c")),
        QStringLiteral("https://github.com/xuhuandeshilinghun/")));
    layout->addWidget(makeLinkLabel(
        I18n::t("about.feedback", QStringLiteral("\xe9\x97\xae\xe9\xa2\x98\xe5\x8f\x8d\xe9\xa6\x88")),
        issuesUrl));

    
    auto* authorTitle = new QLabel(I18n::t("about.author", QStringLiteral("\xe4\xbd\x9c\xe8\x80\x85")), this);
    authorTitle->setFont(QFont("Microsoft YaHei", 11, QFont::Bold));
    layout->addWidget(authorTitle);

    auto* authorLabel = new QLabel(
        I18n::t("about.author_name", QStringLiteral("\xe8\x99\x9a\xe5\xb9\xbb\xe7\x9a\x84\xe6\x98\xaf\xe7\xb5\xbd\xe9\xad\x82")), this);
    authorLabel->setFont(QFont("Microsoft YaHei", 10));
    layout->addWidget(authorLabel);

    auto* emailLabel = new QLabel(
        QStringLiteral("%1: senjin@foxmail.com / moyuliu39@gmail.com")
            .arg(I18n::t("about.email", QStringLiteral("\xe9\x82\xae\xe7\xae\xb1"))), this);
    emailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    emailLabel->setFont(QFont("Microsoft YaHei", 10));
    layout->addWidget(emailLabel);

    layout->addStretch();

    
    auto* copyrightLabel = new QLabel(
        I18n::t("about.copyright", QStringLiteral("\xc2\xa9 2026 SnipOCR")), this);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setFont(QFont("Microsoft YaHei", 9));
    layout->addWidget(copyrightLabel);

    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    m_btnLicense = new QPushButton(I18n::t("about.open_source_license", QStringLiteral("\xe5\xbc\x80\xe6\xba\x90\xe8\xae\xb8\xe5\x8f\xaf")), this);
    m_btnLicense->setObjectName("btnLicense");
    m_btnLicense->setFixedWidth(120);
    connect(m_btnLicense, &QPushButton::clicked, [this]() {
        LicenseDialog dlg(this);
        dlg.exec();
    });
    btnLayout->addWidget(m_btnLicense);
    btnLayout->addStretch();
    m_btnClose = new QPushButton(I18n::t("common.close", QStringLiteral("\xe5\x85\xb3\xe9\x97\xad")), this);
    m_btnClose->setFixedWidth(100);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_btnClose);
    layout->addLayout(btnLayout);
}

void AboutDialog::applyTheme()
{
    bool dark = UiTheme::is_dark_theme();
    QString bg = dark ? "#1e1e1e" : "#f5f5f5";
    QString text = dark ? "#ffffff" : "#333333";
    QString border = dark ? "#3d3d3d" : "#cccccc";
    QString accent = dark ? "#007acc" : "#0078d4";
    QString accentHover = dark ? "#1a8cff" : "#106ebe";

    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; background: transparent; }"
        "QFrame { color: %3; }"
        "QPushButton { background: %4; color: #ffffff; border: none; border-radius: 4px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %5; }"
        "QPushButton#btnLicense { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton#btnLicense:hover { background: %5; color: #ffffff; }"
    ).arg(bg, text, border, accent, accentHover));
}

void AboutDialog::centerOnScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->availableGeometry();
    int x = geo.x() + (geo.width() - width()) / 2;
    int y = geo.y() + (geo.height() - height()) / 2;
    move(x, y);
}



/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "license_dialog.h"
#include "core/i18n.h"
#include "core/ui_theme.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFont>
#include <QMessageBox>
#include <QDesktopServices>

LicenseDialog::LicenseDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::t("license.title", QStringLiteral("\xe5\xbc\x80\xe6\xba\x90\xe5\x8d\x8f\xe8\xae\xae")));
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setFixedSize(560, 420);

    buildUi();
    applyTheme();
    centerOnScreen();
}

void LicenseDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 12);
    layout->setSpacing(10);

    // Header + About Qt button
    auto* topRow = new QHBoxLayout();
    auto* header = new QLabel(I18n::t("license.header", QStringLiteral("SnipOCR open source:")), this);
    header->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    topRow->addWidget(header);
    topRow->addStretch();
    layout->addLayout(topRow);

    // License text
    m_text = new QTextBrowser(this);
    m_text->setReadOnly(true);
    m_text->setFont(QFont("Consolas", 9));

    QString content = QStringLiteral(
        "<div style='line-height:1.6'>"

        "<p><a href='https://www.qt.io' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>Qt</a><br>"
        "<span style='color:%2;'>License:</span> GNU LGPL v3<br>"
        "<span style='color:%2;'>Copyright:</span> (C) The Qt Company Ltd.<br>"
        "<span style='color:%2;'>Description:</span> Cross-platform C++ GUI framework for desktop and embedded applications. "
        "Allows dynamic linking for closed-source commercial software without open-sourcing your own code.</p>"

        "<p><a href='https://github.com/microsoft/onnxruntime' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>ONNX Runtime</a><br>"
        "<span style='color:%2;'>License:</span> MIT License<br>"
        "<span style='color:%2;'>Copyright:</span> (c) Microsoft Corporation<br>"
        "<span style='color:%2;'>Description:</span> Cross-platform inference engine for ONNX AI models. "
        "Supports CPU, GPU and NPU acceleration. Permissive license for closed-source commercial use.</p>"

        "<p><a href='https://opencv.org' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>OpenCV</a><br>"
        "<span style='color:%2;'>License:</span> Apache License 2.0<br>"
        "<span style='color:%2;'>Copyright:</span> (C) 2000-2024, OpenCV team<br>"
        "<span style='color:%2;'>Description:</span> Open source computer vision library, including image processing and visual algorithms. "
        "Suitable for commercial products with patent protection.</p>"

        "<p><a href='https://github.com/dlbeer/quirc' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>quirc</a><br>"
        "<span style='color:%2;'>License:</span> ISC License<br>"
        "<span style='color:%2;'>Copyright:</span> (c) Daniel Beer<br>"
        "<span style='color:%2;'>Description:</span> Lightweight pure-C QR code decoder, no external dependencies. "
        "Permissive license for commercial use.</p>"

        "<p><a href='https://www.zlib.net' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>zlib</a><br>"
        "<span style='color:%2;'>License:</span> zlib License<br>"
        "<span style='color:%2;'>Copyright:</span> (C) 1995-2024 Jean-loup Gailly and Mark Adler<br>"
        "<span style='color:%2;'>Description:</span> Lossless data compression library implementing deflate algorithm, "
        "widely used in image and file compression.</p>"

        "<p><a href='https://learn.microsoft.com/en-us/windows/win32/seccng/cng-portal' style='color:%1;text-decoration:none;font-weight:bold;font-size:11pt;'>Microsoft Windows SDK (CNG)</a><br>"
        "<span style='color:%2;'>Copyright:</span> (c) Microsoft Corporation<br>"
        "<span style='color:%2;'>Description:</span> Windows native development SDK. CNG (Cryptography Next Generation) provides "
        "Windows cryptographic APIs for encryption, decryption and digital signature. "
        "Proprietary component, not open source.</p>"

        "</div>"
    );
        bool dark = UiTheme::is_dark_theme();
    QString linkColor = dark ? "#007acc" : "#0078d4";
    QString labelColor = dark ? "#888888" : "#888888";
    m_text->setHtml(content.arg(linkColor, labelColor));

    // Open links in external browser automatically
    m_text->setOpenExternalLinks(true);
    layout->addWidget(m_text);

    // OK button
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_btnOk = new QPushButton(I18n::t("license.ok", QStringLiteral("OK")), this);
    m_btnOk->setFixedWidth(100);
    connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(m_btnOk);
    layout->addLayout(btnRow);
}

void LicenseDialog::applyTheme()
{
    bool dark = UiTheme::is_dark_theme();
    QString bg = dark ? "#1e1e1e" : "#f5f5f5";
    QString text = dark ? "#ffffff" : "#333333";
    QString inputBg = dark ? "#252525" : "#ffffff";
    QString border = dark ? "#3d3d3d" : "#cccccc";
    QString accent = dark ? "#007acc" : "#0078d4";
    QString accentHover = dark ? "#1a8cff" : "#106ebe";

    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; background: transparent; }"
        "QTextBrowser { background: %3; color: %2; border: 1px solid %4; border-radius: 4px; }"
        "QPushButton {"
        "  background: %5; color: #ffffff;"
        "  border: none; border-radius: 4px;"
        "  padding: 6px 16px; font-size: 13px;"
        "}"
        "QPushButton:hover { background: %6; }"
    ).arg(bg, text, inputBg, border, accent, accentHover));
}

void LicenseDialog::centerOnScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->availableGeometry();
    int x = geo.x() + (geo.width() - width()) / 2;
    int y = geo.y() + (geo.height() - height()) / 2;
    move(x, y);
}

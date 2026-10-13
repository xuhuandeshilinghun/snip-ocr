/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "ocr_result_dialog.h"
#include "core/i18n.h"
#include "core/ui_theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QScreen>

OCRResultDialog::OCRResultDialog(const QString& text, QWidget* parent)
    : QDialog(parent)
    , m_text(text)
{
    setWindowTitle(I18n::t("ocr.result_title", "OCR Result"));
    setFixedSize(500, 400);
    buildUi();
    applyTheme();
    positionBottomRight();
}

void OCRResultDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);

    m_infoLabel = new QLabel(QStringLiteral("  %1").arg(
        I18n::t("ocr.chars_count", "{count} chars").replace("{count}", QString::number(m_text.length()))));
    layout->addWidget(m_infoLabel);

    m_textEdit = new QTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->setPlainText(m_text);
    layout->addWidget(m_textEdit);

    auto* btnLayout = new QHBoxLayout();
    m_btnCopy = new QPushButton(I18n::t("ocr.copy_result", "Copy Result"));
    m_btnClose = new QPushButton(I18n::t("common.close", "Close"));
    connect(m_btnCopy, &QPushButton::clicked, this, &OCRResultDialog::onCopy);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCopy);
    btnLayout->addWidget(m_btnClose);
    layout->addLayout(btnLayout);
}

void OCRResultDialog::applyTheme()
{
    bool dark = UiTheme::is_dark_theme();
    QString bg = dark ? "#1e1e1e" : "#f0f0f0";
    QString text = dark ? "#ffffff" : "#333333";
    QString border = dark ? "#3d3d3d" : "#cccccc";
    QString inputBg = dark ? "#2d2d2d" : "#ffffff";
    QString btnBg = dark ? "#007acc" : "#0078d4";
    QString btnHover = dark ? "#1a8cff" : "#106ebe";

    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; }"
        "QTextEdit { background: %3; color: %2; border: 1px solid %4; border-radius: 4px;"
        "  font-size: 14px; font-family: Consolas, \"Microsoft YaHei\", monospace; }"
        "QPushButton { background: %5; color: #ffffff; border: none; border-radius: 4px;"
        "  padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %6; }")
        .arg(bg).arg(text).arg(inputBg).arg(border).arg(btnBg).arg(btnHover));
}

void OCRResultDialog::positionBottomRight()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->availableGeometry();
    int x = geo.right() - width() - 16;
    int y = geo.bottom() - height() - 16;
    move(x, y);
}

void OCRResultDialog::updateText(const QString& text, const QString& title)
{
    m_text = text;
    if (!title.isEmpty())
        setWindowTitle(title);
    m_infoLabel->setText(QStringLiteral("  %1").arg(
        I18n::t("ocr.chars_count", "{count} chars").replace("{count}", QString::number(text.length()))));
    m_textEdit->setPlainText(text);
    m_btnCopy->setText(I18n::t("ocr.copy_result", "Copy Result"));
    positionBottomRight();
}

void OCRResultDialog::onCopy()
{
    QApplication::clipboard()->setText(m_text);
    m_btnCopy->setText(I18n::t("msg.copied", "Copied"));
}

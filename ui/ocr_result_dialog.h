/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QDialog>
#include <QString>

class QTextEdit;
class QLabel;
class QPushButton;

// OCR result dialog: shows recognized text, copy/close buttons.
// Faithfully ports Python ui/ocr_result_dialog.py.
class OCRResultDialog : public QDialog {
    Q_OBJECT
public:
    explicit OCRResultDialog(const QString& text, QWidget* parent = nullptr);

    // Reuse window: update text content without creating a new window.
    void updateText(const QString& text, const QString& title = QString());

private:
    void buildUi();
    void applyTheme();
    void positionBottomRight();
    void onCopy();

    QString m_text;
    QLabel* m_infoLabel = nullptr;
    QTextEdit* m_textEdit = nullptr;
    QPushButton* m_btnCopy = nullptr;
    QPushButton* m_btnClose = nullptr;
};

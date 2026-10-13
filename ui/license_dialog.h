/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QDialog>

class QTextBrowser;
class QPushButton;

class LicenseDialog : public QDialog {
    Q_OBJECT
public:
    explicit LicenseDialog(QWidget* parent = nullptr);
    ~LicenseDialog() override = default;

private:
    void buildUi();
    void applyTheme();
    void centerOnScreen();

    QTextBrowser* m_text = nullptr;
    QPushButton* m_btnOk = nullptr;
};

#pragma once
#include <QDialog>

class QLabel;
class QPushButton;


class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override = default;

private:
    void buildUi();
    void applyTheme();
    void centerOnScreen();

    QLabel* makeLinkLabel(const QString& text, const QString& url);

    QPushButton* m_btnClose = nullptr;
    QPushButton* m_btnLicense = nullptr;
};


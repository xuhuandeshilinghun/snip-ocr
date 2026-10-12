#include "settings_dialog.h"
#include "core/i18n.h"
#include "core/config_manager.h"
#include "core/ui_theme.h"
#include "core/hotkey_manager.h"
#include "core/img_res_loader.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QTabWidget>
#include <QLabel>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QBuffer>
#include <QProxyStyle>
#include <QStyleOption>
#include <QKeySequence>
#include <QByteArray>
#include <QVariant>
#include <QJsonObject>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QDir>


// ─── Checkmark proxy style: draws checkmark from img_res.bin (no disk file) ───
class CheckmarkStyle : public QProxyStyle {
public:
    static const QPixmap& checkmarkPixmap() {
        static QPixmap pm;
        if (pm.isNull()) {
            QByteArray svgData = ImgResLoader::get_img_bytes("check.svg");
            if (!svgData.isEmpty()) {
                QSvgRenderer renderer(svgData);
                if (renderer.isValid()) {
                    QImage img(16, 16, QImage::Format_ARGB32_Premultiplied);
                    img.fill(Qt::transparent);
                    QPainter painter(&img);
                    painter.setRenderHint(QPainter::Antialiasing);
                    renderer.render(&painter);
                    painter.end();
                    pm = QPixmap::fromImage(img);
                }
            }
        }
        return pm;
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget = nullptr) const override {
        if (element == PE_IndicatorCheckBox) {
            bool checked = option->state & State_On;
            bool dark = UiTheme::is_dark_theme();

            QRect r = option->rect;
            r.setWidth(16);
            r.setHeight(16);

            // Draw border + background
            painter->setRenderHint(QPainter::Antialiasing);
            if (checked) {
                QColor accent(dark ? "#007acc" : "#0078d4");
                painter->setPen(QPen(accent, 1));
                painter->setBrush(QBrush(accent));
            } else {
                QColor border(dark ? "#3d3d3d" : "#cccccc");
                QColor bg(dark ? "#252525" : "#ffffff");
                painter->setPen(QPen(border, 1));
                painter->setBrush(QBrush(bg));
            }
            painter->drawRoundedRect(r, 3, 3);

            // Draw checkmark if checked
            if (checked) {
                const QPixmap& pm = checkmarkPixmap();
                if (!pm.isNull()) {
                    painter->drawPixmap(
                        r.center().x() - pm.width() / 2,
                        r.center().y() - pm.height() / 2,
                        pm);
                }
            }
        } else {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }
};
#include <QFileInfo>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <wrl/client.h>
#include <comdef.h>
#endif

// SVG pixmaps for hotkey validation icons (same as Python _SVG_OK/_SVG_FAIL).
static const char* kSvgOk =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1024 1024\" width=\"20\" height=\"20\">"
    "<path d=\"M512.186242 6.139837c-278.100856 0-504.362043 226.261187-504.362043 504.362043S234.085386 1014.863922 512.186242 1014.863922s504.362043-226.261187 504.362043-504.362043S790.287097 6.139837 512.186242 6.139837zM794.409998 413.799444 466.418872 750.040465c-7.838525 8.023744-18.590404 12.579503-29.816073 12.642948l-0.227174 0c-11.141758 0-21.834284-4.433986-29.715789-12.314467L230.289943 574.038962c-16.420995-16.396435-16.420995-43.013653 0-59.430554 16.416901-16.420995 43.013653-16.420995 59.430554 0l146.32767 146.281621 298.275337-305.866223c16.253172-16.58063 42.891879-16.891715 59.430554-0.655939C810.356179 370.600573 810.646797 397.21779 794.409998 413.799444z\" fill=\"#1afa29\"/>"
    "</svg>";
static const char* kSvgFail =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1024 1024\" width=\"20\" height=\"20\">"
    "<path d=\"M358.419788 392.527249c0-37.683671-30.557838-68.26503-68.256849-68.26503-37.700034 0-68.257872 30.582382-68.257872 68.26503 0 37.715373 30.557838 68.26503 68.257872 68.26503C327.86195 460.793302 358.419788 430.242622 358.419788 392.527249zM733.847287 324.262218c-37.699011 0-68.267076 30.582382-68.267076 68.26503 0 37.715373 30.568065 68.26503 68.267076 68.26503s68.256849-30.55068 68.256849-68.26503C802.104137 354.843577 771.546298 324.262218 733.847287 324.262218zM511.999489 0c-282.737469 0-511.94222 229.212932-511.94222 511.991307s229.204751 512.008693 511.94222 512.008693 511.943243-229.229295 511.943243-512.008693S794.736958 0 511.999489 0zM511.999489 972.783586c-254.462188 0-460.743192-206.313728-460.743192-460.792279 0-254.478551 206.279981-460.775917 460.743192-460.775917 254.463211 0 460.75035 206.297366 460.75035 460.775917C972.749839 766.469858 766.463722 972.783586 511.999489 972.783586zM512.066984 597.339725c-154.214767 0-204.796114 136.532106-204.796114 136.532106l51.199028 0c0 0 39.032548-85.331032 153.597085-85.331032 116.072948 0 153.597085 85.331032 153.597085 85.331032l51.198006 0C716.863098 733.871831 666.28175 597.339725 512.066984 597.339725z\" fill=\"#d81e06\"/>"
    "</svg>";

QSet<QString> SettingsDialog::s_systemConflicts;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::t("settings.title", QStringLiteral("\xe8\xae\xbe\xe7\xbd\xae")));
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setFixedSize(480, 420);

    if (s_systemConflicts.isEmpty()) {
        s_systemConflicts
                          << "Ctrl+C" << "Ctrl+V" << "Ctrl+X" << "Ctrl+Z" << "Ctrl+A"
                          << "Ctrl+S" << "Ctrl+P" << "Ctrl+F" << "Ctrl+H" << "Ctrl+O"
                          << "Ctrl+W" << "Ctrl+N" << "Ctrl+T"
                          << "Ctrl+Shift+Esc" << "Alt+Tab" << "Alt+F4"
                          << "Win+E" << "Win+D" << "Win+L" << "Win+R" << "Win+I" << "Print";
    }

    setStyle(new CheckmarkStyle());
    buildUi();
    initPixmaps();
    loadValues();
    applyTheme(UiTheme::is_dark_theme());
    centerOnScreen();
}

void SettingsDialog::setHotkeyManager(GlobalHotkeyManager* mgr)
{
    m_hotkeyMgr = mgr;
}

void SettingsDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    connect(m_tabs, &QTabWidget::currentChanged, this, &SettingsDialog::onTabChanged);
    layout->addWidget(m_tabs, 1);

    buildGeneralTab();
    buildControlTab();
    buildMagnifierTab();
    buildButtonRow();
}

void SettingsDialog::buildGeneralTab()
{
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(20, 16, 20, 16);
    form->setLabelAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);

    m_comboLang = new QComboBox(page);
    m_comboLang->setMinimumWidth(200);
    m_comboLang->addItem(QStringLiteral("简体中文"), "zh_CN");
    m_comboLang->addItem(QStringLiteral("繁體中文"), "zh_TW");
    m_comboLang->addItem(QStringLiteral("English"), "en_US");
    m_comboLang->addItem(QStringLiteral("日本語"), "ja");
    m_comboLang->addItem(QStringLiteral("한국어"), "ko");
    m_comboLang->addItem(QStringLiteral("Deutsch"), "de");
    m_comboLang->addItem(QStringLiteral("Français"), "fr");
    m_comboLang->addItem(QStringLiteral("Español"), "es");
    m_comboLang->addItem(QStringLiteral("Português"), "pt");
    m_comboLang->addItem(QStringLiteral("Русский"), "ru");
    form->addRow(I18n::t("settings.language", QStringLiteral("\xe6\x98\xbe\xe7\xa4\xba\xe8\xaf\xad\xe8\xa8\x80")), m_comboLang);

    m_comboTheme = new QComboBox(page);
    m_comboTheme->setMinimumWidth(200);
    m_comboTheme->addItem(I18n::t("settings.theme_dark", QStringLiteral("\xe6\xb7\xb1\xe8\x89\xb2")), "dark");
    m_comboTheme->addItem(I18n::t("settings.theme_light", QStringLiteral("\xe6\xb5\x85\xe8\x89\xb2")), "light");
    m_comboTheme->addItem(I18n::t("settings.theme_system", QStringLiteral("\xe8\xb7\x9f\xe9\x9a\x8f\xe7\xb3\xbb\xe7\xbb\x9f")), "system");
    connect(m_comboTheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeChanged);
    form->addRow(I18n::t("settings.theme", QStringLiteral("\xe4\xb8\xbb\xe9\xa2\x98")), m_comboTheme);

    form->addItem(new QSpacerItem(0, 8, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_checkTray = new QCheckBox(I18n::t("settings.minimize_to_tray",
            QStringLiteral("\xe6\x9c\x80\xe5\xb0\x8f\xe5\x8c\x96\xe5\x88\xb0\xe6\x89\x98\xe7\x9b\x98")), page);
    form->addRow(QString(), m_checkTray);

    m_checkAutostart = new QCheckBox(I18n::t("settings.auto_start",
            QStringLiteral("\xe5\xbc\x80\xe6\x9c\xba\xe5\x90\xaf\xe5\x8a\xa8")), page);
    form->addRow(QString(), m_checkAutostart);

    form->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_tabs->addTab(page, I18n::t("settings.tab_general", QStringLiteral("\xe5\xb8\xb8\xe8\xa7\x84")));
}

void SettingsDialog::buildControlTab()
{
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    m_subTabs = new QTabWidget(page);
    pageLayout->addWidget(m_subTabs, 1);

    // Behavior sub-tab
    auto* behaviorPage = new QWidget(this);
    auto* bform = new QFormLayout(behaviorPage);
    bform->setContentsMargins(20, 16, 20, 16);
    bform->setLabelAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    bform->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    bform->setHorizontalSpacing(16);
    bform->setVerticalSpacing(10);

    m_comboDelay = new QComboBox(behaviorPage);
    m_comboDelay->setMinimumWidth(100);
    m_comboDelay->addItem("3 " + I18n::t("settings.seconds", QStringLiteral("\xe7\xa7\x92")), 3);
    m_comboDelay->addItem("5 " + I18n::t("settings.seconds", QStringLiteral("\xe7\xa7\x92")), 5);
    m_comboDelay->addItem("10 " + I18n::t("settings.seconds", QStringLiteral("\xe7\xa7\x92")), 10);
    m_checkDelayLoop = new QCheckBox(I18n::t("settings.delay_loop", QStringLiteral("\xe5\xbe\xaa\xe7\x8e\xaf")), behaviorPage);
    auto* delayRow = new QHBoxLayout();
    delayRow->setSpacing(12);
    delayRow->addWidget(m_comboDelay);
    delayRow->addWidget(m_checkDelayLoop);
    delayRow->addStretch();
    auto* delayWidget = new QWidget(behaviorPage);
    delayWidget->setLayout(delayRow);
    bform->addRow(I18n::t("settings.delay_seconds",
            QStringLiteral("\xe5\xbb\xb6\xe6\x97\xb6\xe7\xa7\x92\xe6\x95\xb0")), delayWidget);

    m_labelDelayHint = new QLabel(I18n::t("settings.delay_seconds_hint",
            QStringLiteral("\xe9\x85\x8d\xe5\x90\x88\xe3\x80\x8c\xe5\xbb\xb6\xe8\xbf\x9f\xe6\x88\xaa\xe5\x9b\xbe\xe3\x80\x8d\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xbd\xbf\xe7\x94\xa8\xef\xbc\x8c\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe8\xaf\xb7\xe5\x9c\xa8\xe3\x80\x8c\xe6\x8e\xa7\xe5\x88\xb6 \xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe3\x80\x8d\xe4\xb8\xad\xe8\xae\xbe\xe7\xbd\xae")), behaviorPage);
    m_labelDelayHint->setStyleSheet("color: gray; font-size: 11px; padding-left: 2px;");
    m_labelDelayHint->setWordWrap(true);
    bform->addRow(QString(), m_labelDelayHint);

    bform->addItem(new QSpacerItem(0, 4, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_comboOcrLang = new QComboBox(behaviorPage);
    m_comboOcrLang->setMinimumWidth(200);
    m_comboOcrLang->addItem(I18n::t("ocr.lang_auto", QStringLiteral("\xe8\x87\xaa\xe5\x8a\xa8\xe6\xa3\x80\xe6\xb5\x8b")), "auto");
    m_comboOcrLang->addItem(I18n::t("ocr.lang_v6", QStringLiteral("\xe4\xb8\xad\xe6\x97\xa5\xe8\x8b\xb1")), "v6");
    m_comboOcrLang->addItem(I18n::t("ocr.lang_ko", QStringLiteral("\xe9\x9f\xa9\xe6\x96\x87")), "ko");
    m_comboOcrLang->addItem(I18n::t("ocr.lang_ru", QStringLiteral("\xe4\xbf\x84\xe6\x96\x87")), "ru");
    m_comboOcrLang->addItem(I18n::t("ocr.lang_vi", QStringLiteral("\xe8\xb6\x8a\xe5\x8d\x97\xe6\x96\x87")), "vi");
    bform->addRow(I18n::t("settings.ocr_lang",
            QStringLiteral("\xe9\xbb\x98\xe8\xae\xa4\xe8\xaf\x86\xe5\x88\xab\xe8\xaf\xad\xe8\xa8\x80")), m_comboOcrLang);

    m_labelOcrHint = new QLabel(I18n::t("settings.ocr_lang_hint",
            QStringLiteral("\xe9\x85\x8d\xe5\x90\x88\xe3\x80\x8cOCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe3\x80\x8d\xe4\xbd\xbf\xe7\x94\xa8\xef\xbc\x8c\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe8\xaf\xb7\xe5\x9c\xa8\xe3\x80\x8c\xe6\x8e\xa7\xe5\x88\xb6 \xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe3\x80\x8d\xe4\xb8\xad\xe8\xae\xbe\xe7\xbd\xae")), behaviorPage);
    m_labelOcrHint->setStyleSheet("color: gray; font-size: 11px; padding-left: 2px;");
    m_labelOcrHint->setWordWrap(true);
    bform->addRow(QString(), m_labelOcrHint);

    bform->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_subTabs->addTab(behaviorPage, I18n::t("settings.tab_behavior", QStringLiteral("\xe8\xa1\x8c\xe4\xb8\xba")));

    // Hotkey sub-tab
    auto* hotkeyPage = new QWidget(this);
    auto* hform = new QFormLayout(hotkeyPage);
    hform->setContentsMargins(20, 16, 20, 16);
    hform->setLabelAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    hform->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    hform->setHorizontalSpacing(16);
    hform->setVerticalSpacing(12);

    m_editSnap = new QKeySequenceEdit(hotkeyPage);
    m_editSnap->setMinimumWidth(200);
    connect(m_editSnap, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::onHotkeyChanged);
    m_iconSnap = new QLabel(hotkeyPage);
    m_iconSnap->setFixedSize(22, 22);
    auto* snapRow = new QHBoxLayout();
    snapRow->setSpacing(8);
    snapRow->addWidget(m_editSnap, 1);
    snapRow->addWidget(m_iconSnap);
    auto* snapWidget = new QWidget(hotkeyPage);
    snapWidget->setLayout(snapRow);
    hform->addRow(I18n::t("settings.snapshot_hotkey",

            QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae")), snapWidget);

    m_editPin = new QKeySequenceEdit(hotkeyPage);
    m_editPin->setMinimumWidth(200);
    connect(m_editPin, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::onHotkeyChanged);
    m_iconPin = new QLabel(hotkeyPage);
    m_iconPin->setFixedSize(22, 22);
    auto* pinRow = new QHBoxLayout();
    pinRow->setSpacing(8);
    pinRow->addWidget(m_editPin, 1);
    pinRow->addWidget(m_iconPin);
    auto* pinWidget = new QWidget(hotkeyPage);
    pinWidget->setLayout(pinRow);
    hform->addRow(I18n::t("settings.pin_hotkey", QStringLiteral("\xe8\xb4\xb4\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae")), pinWidget);

    m_editOcr = new QKeySequenceEdit(hotkeyPage);
    m_editOcr->setMinimumWidth(200);
    connect(m_editOcr, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::onHotkeyChanged);
    m_iconOcr = new QLabel(hotkeyPage);
    m_iconOcr->setFixedSize(22, 22);
    auto* ocrRow = new QHBoxLayout();
    ocrRow->setSpacing(8);
    ocrRow->addWidget(m_editOcr, 1);
    ocrRow->addWidget(m_iconOcr);
    auto* ocrWidget = new QWidget(hotkeyPage);
    ocrWidget->setLayout(ocrRow);
    hform->addRow(I18n::t("settings.ocr_hotkey",
            QStringLiteral("OCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae")), ocrWidget);

    m_editDelay = new QKeySequenceEdit(hotkeyPage);
    m_editDelay->setMinimumWidth(200);
    connect(m_editDelay, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::onHotkeyChanged);
    m_iconDelay = new QLabel(hotkeyPage);
    m_iconDelay->setFixedSize(22, 22);
    auto* delayRow2 = new QHBoxLayout();
    delayRow2->setSpacing(8);
    delayRow2->addWidget(m_editDelay, 1);
    delayRow2->addWidget(m_iconDelay);
    auto* delayWidget2 = new QWidget(hotkeyPage);
    delayWidget2->setLayout(delayRow2);
    hform->addRow(I18n::t("settings.delay_hotkey",
            QStringLiteral("\xe5\xbb\xb6\xe8\xbf\x9f\xe6\x88\xaa\xe5\x9b\xbe")), delayWidget2);

    hform->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_subTabs->addTab(hotkeyPage, I18n::t("settings.tab_hotkey", QStringLiteral("\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae")));

    m_tabs->addTab(page, I18n::t("settings.tab_control", QStringLiteral("\xe6\x8e\xa7\xe5\x88\xb6")));
}

void SettingsDialog::buildMagnifierTab()
{
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(20, 16, 20, 16);
    form->setLabelAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);

    m_comboMagColor = new QComboBox(page);
    m_comboMagColor->setMinimumWidth(200);
    m_comboMagColor->addItem(I18n::t("settings.mag_color_none", QStringLiteral("\xe6\x97\xa0")), QString::fromUtf8("\xe6\x97\xa0"));
    m_comboMagColor->addItem("RGB", "RGB");
    m_comboMagColor->addItem("HEX", "HEX");
    m_comboMagColor->addItem("RGB + HEX", "RGB+HEX");
    m_comboMagColor->addItem("HEX + RGB", "HEX+RGB");
    form->addRow(I18n::t("settings.mag_color_format",
            QStringLiteral("\xe9\xa2\x9c\xe8\x89\xb2\xe5\x80\xbc\xe6\xa0\xbc\xe5\xbc\x8f")), m_comboMagColor);

    form->addItem(new QSpacerItem(0, 8, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_checkMagEnabled = new QCheckBox(I18n::t("settings.mag_enabled",
            QStringLiteral("\xe6\x98\xbe\xe7\xa4\xba\xe6\x94\xbe\xe5\xa4\xa7\xe9\x95\x9c")), page);
    form->addRow(QString(), m_checkMagEnabled);

    m_checkMagHideHash = new QCheckBox(I18n::t("settings.mag_hide_hash",
            QStringLiteral("\xe9\x9a\x90\xe8\x97\x8f '#'")), page);
    form->addRow(QString(), m_checkMagHideHash);

    m_checkMagHexUpper = new QCheckBox(I18n::t("settings.mag_hex_upper",
            QStringLiteral("Hex \xe9\xa2\x9c\xe8\x89\xb2\xe5\x80\xbc\xe5\xa4\xa7\xe5\x86\x99")), page);
    form->addRow(QString(), m_checkMagHexUpper);

    auto* restoreRow = new QHBoxLayout();
    restoreRow->addStretch();
    auto* btnRestore = new QPushButton(I18n::t("settings.mag_restore",
            QStringLiteral("\xe6\x81\xa2\xe5\xa4\x8d\xe9\xbb\x98\xe8\xae\xa4")), page);
    connect(btnRestore, &QPushButton::clicked, this, &SettingsDialog::onRestoreMagDefaults);
    restoreRow->addWidget(btnRestore);
    form->addRow(restoreRow);

    form->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_tabs->addTab(page, I18n::t("settings.tab_magnifier",
            QStringLiteral("\xe6\x94\xbe\xe5\xa4\xa7\xe9\x95\x9c")));
}

void SettingsDialog::buildButtonRow()
{
    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!layout) return;
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(20, 8, 20, 12);
    m_btnSave = new QPushButton(I18n::t("settings.save",
            QStringLiteral("\xe4\xbf\x9d\xe5\xad\x98")), this);
    m_btnCancel = new QPushButton(I18n::t("settings.cancel",
            QStringLiteral("\xe5\x8f\x96\xe6\xb6\x88")), this);
    m_btnSave->setMinimumWidth(80);
    m_btnCancel->setMinimumWidth(80);
    connect(m_btnSave, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnSave);
    btnLayout->addWidget(m_btnCancel);
    layout->addLayout(btnLayout);
}

void SettingsDialog::initPixmaps()
{
    
    static QPixmap s_pmOk;
    static QPixmap s_pmFail;
    static bool s_init = false;
    if (!s_init) {
        s_init = true;
        const int size = 20;
        for (auto item : {std::make_pair(kSvgOk, &s_pmOk),
                           std::make_pair(kSvgFail, &s_pmFail)}) {
            QSvgRenderer renderer(QByteArray(item.first));
            QImage img(size, size, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter p(&img);
            renderer.render(&p);
            p.end();
            *item.second = QPixmap::fromImage(img);
        }
    }
    m_pixmapOk = s_pmOk;
    m_pixmapFail = s_pmFail;
}

void SettingsDialog::loadValues()
{
    auto& cfg = ConfigManager::instance();

    QString lang = cfg.get("language", "zh_CN").toString();
    for (int i = 0; i < m_comboLang->count(); ++i)
        if (m_comboLang->itemData(i).toString() == lang) { m_comboLang->setCurrentIndex(i); break; }

    QString theme = cfg.get("theme", "dark").toString();
    for (int i = 0; i < m_comboTheme->count(); ++i)
        if (m_comboTheme->itemData(i).toString() == theme) { m_comboTheme->setCurrentIndex(i); break; }

    m_checkTray->setChecked(cfg.get("minimize_to_tray", true).toBool());
    m_checkAutostart->setChecked(isAutoStartEnabled());

    QVariant hkVar = cfg.get("hotkey");
    QJsonObject hk = hkVar.toJsonObject();
    if (hk.isEmpty()) {
        QVariantMap vm = hkVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            hk.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    m_editSnap->setKeySequence(QKeySequence(hk.value("snapshot").toString("F1")));
    m_editPin->setKeySequence(QKeySequence(hk.value("pin").toString("F2")));
    m_editOcr->setKeySequence(QKeySequence(hk.value("ocr").toString("Ctrl+Shift+O")));
    m_editDelay->setKeySequence(QKeySequence(hk.value("delay_snapshot").toString("Ctrl+Shift+D")));

    
    QString ocrModelPath = QDir(QCoreApplication::applicationDirPath()).filePath("model/MediaOCR-Base-v1.mtk-model");
    bool ocrAvailable = QFileInfo::exists(ocrModelPath);
    if (!ocrAvailable) {
        m_editOcr->setEnabled(false);
        if (m_iconOcr) m_iconOcr->setVisible(false);
        if (m_comboOcrLang) m_comboOcrLang->setEnabled(false);
        if (m_labelOcrHint) m_labelOcrHint->setEnabled(false);
    }

    int delaySec = cfg.get("delay_seconds", 5).toInt();
    for (int i = 0; i < m_comboDelay->count(); ++i)
        if (m_comboDelay->itemData(i).toInt() == delaySec) { m_comboDelay->setCurrentIndex(i); break; }
    m_checkDelayLoop->setChecked(cfg.get("delay_loop", false).toBool());

    QString ocrLang = cfg.get("ocr_rec_lang", "auto").toString();
    for (int i = 0; i < m_comboOcrLang->count(); ++i)
        if (m_comboOcrLang->itemData(i).toString() == ocrLang) { m_comboOcrLang->setCurrentIndex(i); break; }

    QVariant magVar = cfg.get("magnifier");
    QJsonObject mag = magVar.toJsonObject();
    if (mag.isEmpty()) {
        QVariantMap vm = magVar.toMap();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            mag.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    m_checkMagEnabled->setChecked(mag.value("enabled").toBool(true));
    QString magFmt = mag.value("color_format").toString("RGB");
    for (int i = 0; i < m_comboMagColor->count(); ++i)
        if (m_comboMagColor->itemData(i).toString() == magFmt) { m_comboMagColor->setCurrentIndex(i); break; }
    m_checkMagHideHash->setChecked(mag.value("hide_hash").toBool(false));
    m_checkMagHexUpper->setChecked(mag.value("hex_uppercase").toBool(false));

    validateHotkeys();
}

void SettingsDialog::onRestoreMagDefaults()
{
    m_checkMagEnabled->setChecked(true);
    m_comboMagColor->setCurrentIndex(1);  // RGB
    m_checkMagHideHash->setChecked(false);
    m_checkMagHexUpper->setChecked(false);
}

void SettingsDialog::onTabChanged(int index)
{
    if (!m_hotkeyMgr) return;
    // Tab 1 is "Control" - disable hotkeys while user is editing hotkeys.
    if (index == 1) m_hotkeyMgr->setEnabled(false);
    else            m_hotkeyMgr->setEnabled(true);
}

void SettingsDialog::onThemeChanged()
{
    QString theme = m_comboTheme->currentData().toString();
    bool dark = (theme == "dark");
    if (theme == "system") dark = UiTheme::is_dark_theme();
    applyTheme(dark);
}

void SettingsDialog::onHotkeyChanged()
{
    validateHotkeys();
}

void SettingsDialog::validateHotkeys()
{
    QString snap = m_editSnap->keySequence().toString();
    QString ocr = m_editOcr->keySequence().toString();
    QString delay = m_editDelay->keySequence().toString();

    bool snapOk = !snap.isEmpty() && snap != ocr && snap != delay && !s_systemConflicts.contains(snap);
    bool ocrOk = !ocr.isEmpty() && ocr != snap && ocr != delay && !s_systemConflicts.contains(ocr);
    bool delayOk = !delay.isEmpty() && delay != snap && delay != ocr && !s_systemConflicts.contains(delay);

    m_iconSnap->setPixmap(snapOk ? m_pixmapOk : m_pixmapFail);
    m_iconOcr->setPixmap(ocrOk ? m_pixmapOk : m_pixmapFail);
    m_iconDelay->setPixmap(delayOk ? m_pixmapOk : m_pixmapFail);
}

QPair<bool, QString> SettingsDialog::checkConflicts() const
{
    QString snap = m_editSnap->keySequence().toString();
    QString ocr = m_editOcr->keySequence().toString();
    QString delay = m_editDelay->keySequence().toString();

    if (snap.isEmpty())
        return qMakePair(false, I18n::t("settings.hotkey_empty_snap",
                QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xb8\x8d\xe8\x83\xbd\xe4\xb8\xba\xe7\xa9\xba")));
    if (ocr.isEmpty())
        return qMakePair(false, I18n::t("settings.hotkey_empty_ocr",
                QStringLiteral("OCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xb8\x8d\xe8\x83\xbd\xe4\xb8\xba\xe7\xa9\xba")));

    if (snap == ocr)
        return qMakePair(false, I18n::t("settings.hotkey_same",
                QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x92\x8cOCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xb8\x8d\xe8\x83\xbd\xe7\x9b\xb8\xe5\x90\x8c\n\xe5\xbd\x93\xe5\x89\x8d\xe9\x83\xbd\xe6\x98\xaf: {key}")).replace("{key}", snap));
    if (snap == delay)
        return qMakePair(false, I18n::t("settings.hotkey_same_snap_delay",
                QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x92\x8c\xe5\xbb\xb6\xe6\x97\xb6\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xb8\x8d\xe8\x83\xbd\xe7\x9b\xb8\xe5\x90\x8c\n\xe5\xbd\x93\xe5\x89\x8d\xe9\x83\xbd\xe6\x98\xaf: {key}")).replace("{key}", snap));
    if (ocr == delay)
        return qMakePair(false, I18n::t("settings.hotkey_same_ocr_delay",
                QStringLiteral("OCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x92\x8c\xe5\xbb\xb6\xe6\x97\xb6\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe4\xb8\x8d\xe8\x83\xbd\xe7\x9b\xb8\xe5\x90\x8c\n\xe5\xbd\x93\xe5\x89\x8d\xe9\x83\xbd\xe6\x98\xaf: {key}")).replace("{key}", ocr));

    if (s_systemConflicts.contains(snap))
        return qMakePair(false, I18n::t("settings.hotkey_sys_conflict_snap",
                QStringLiteral("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae [{key}] \xe4\xb8\x8e Windows \xe7\xb3\xbb\xe7\xbb\x9f\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x86\xb2\xe7\xaa\x81:\n  {name}")).replace("{key}", snap));
    if (s_systemConflicts.contains(ocr))
        return qMakePair(false, I18n::t("settings.hotkey_sys_conflict_ocr",
                QStringLiteral("OCR\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae [{key}] \xe4\xb8\x8e Windows \xe7\xb3\xbb\xe7\xbb\x9f\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x86\xb2\xe7\xaa\x81:\n  {name}")).replace("{key}", ocr));
    if (s_systemConflicts.contains(delay))
        return qMakePair(false, I18n::t("settings.hotkey_sys_conflict_delay",
                QStringLiteral("\xe5\xbb\xb6\xe6\x97\xb6\xe6\x88\xaa\xe5\x9b\xbe\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae [{key}] \xe4\xb8\x8e Windows \xe7\xb3\xbb\xe7\xbb\x9f\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x86\xb2\xe7\xaa\x81:\n  {name}")).replace("{key}", delay));

    return qMakePair(true, QString());
}

void SettingsDialog::onSave()
{
    auto [ok, msg] = checkConflicts();
    if (!ok) {
        QMessageBox::warning(this, I18n::t("settings.hotkey_conflict",
                QStringLiteral("\xe5\xbf\xab\xe6\x8d\xb7\xe9\x94\xae\xe5\x86\xb2\xe7\xaa\x81")), msg);
        return;
    }

    auto& cfg = ConfigManager::instance();
    cfg.set("language", m_comboLang->currentData());
    cfg.set("theme", m_comboTheme->currentData());
    cfg.set("minimize_to_tray", m_checkTray->isChecked());

    // Auto-start: create/remove Startup folder shortcut
    bool autoStart = m_checkAutostart->isChecked();
    cfg.set("auto_start", autoStart);
    syncAutoStart(autoStart);

    QJsonObject hk;
    hk["snapshot"] = m_editSnap->keySequence().toString();
    hk["pin"] = m_editPin->keySequence().toString();
    hk["ocr"] = m_editOcr->keySequence().toString();
    hk["delay_snapshot"] = m_editDelay->keySequence().toString();
    cfg.set("hotkey", hk);

    cfg.set("delay_seconds", m_comboDelay->currentData().toInt());
    cfg.set("delay_loop", m_checkDelayLoop->isChecked());
    cfg.set("ocr_rec_lang", m_comboOcrLang->currentData());

    QJsonObject mag;
    mag["enabled"] = m_checkMagEnabled->isChecked();
    mag["color_format"] = m_comboMagColor->currentData().toString();
    mag["hide_hash"] = m_checkMagHideHash->isChecked();
    mag["hex_uppercase"] = m_checkMagHexUpper->isChecked();
    cfg.set("magnifier", mag);

    emit settingsChanged();
    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}

void SettingsDialog::applyTheme(bool dark)
{
    QString bg = dark ? "#1e1e1e" : "#f0f0f0";
    QString text = dark ? "#ffffff" : "#333333";
    QString border = dark ? "#3d3d3d" : "#cccccc";
    QString inputBg = dark ? "#2d2d2d" : "#ffffff";
    QString panelBg = dark ? "#2d2d2d" : "#ffffff";
    QString accent = dark ? "#007acc" : "#0078d4";
    QString selectBg = dark ? "#3d3d3d" : "#cce4f7";
    QString btnBg = dark ? "#3d3d3d" : "#e5e5e5";
    QString btnHover = dark ? "#4d4d4d" : "#d0d0d0";
    QString btnText = dark ? "#ffffff" : "#333333";

    setUpdatesEnabled(false);
    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QDialog QWidget { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QComboBox, QKeySequenceEdit, QLineEdit {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 4px; padding: 4px;"
        "  min-height: 22px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: %3; color: %2;"
        "  selection-background-color: %5;"
        "  border: 1px solid %4;"
        "}"
        "QCheckBox { color: %2; spacing: 6px; background: transparent; }"
        "QPushButton {"
        "  background: %7; color: %8;"
        "  border: 1px solid %4;"
        "  border-radius: 4px;"
        "  padding: 6px 16px; font-size: 13px;"
        "}"
        "QPushButton:hover { background: %9; }"
        "QTabWidget::pane { border: 1px solid %4; background: %10; }"
        "QTabBar::tab {"
        "  background: %1; color: %2;"
        "  padding: 6px 16px;"
        "  border: 1px solid %4; border-bottom: none;"
        "  border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected { background: %6; color: #ffffff; }"
        "QTabBar::tab:hover:!selected { background: %5; }"
        "QTabWidget QTabWidget QTabBar::tab {"
        "  font-size: 11px;"
        "  padding: 4px 12px;"
        "  background: %10;"
        "  border: 1px solid %4;"
        "  border-bottom: 2px solid %6;"
        "  border-top-left-radius: 3px;"
        "  border-top-right-radius: 3px;"
        "  margin-right: 1px;"
        "}"
        "QTabWidget QTabWidget QTabBar::tab:selected {"
        "  background: %6; color: #ffffff;"
        "  border-bottom: 2px solid %6;"
        "}"
        "QTabWidget QTabWidget QTabBar::tab:hover:!selected { background: %5; }"
        "QTabWidget QTabWidget::pane { border: none; background: %1; }"
    ).arg(bg, text, inputBg, border, selectBg, accent,
          btnBg, btnText, btnHover, panelBg));
    repaint();
    setUpdatesEnabled(true);
}

void SettingsDialog::centerOnScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect geo = screen->availableGeometry();
    int x = geo.x() + (geo.width() - width()) / 2;
    int y = geo.y() + (geo.height() - height()) / 2;
    move(x, y);
}

// ==================== Auto-start (Windows Startup folder shortcut) ====================

QString SettingsDialog::getStartupDir()
{
#ifdef Q_OS_WIN
    wchar_t path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, path))) {
        return QString::fromWCharArray(path);
    }
#endif
    return QString();
}

QString SettingsDialog::getShortcutPath()
{
    QString dir = getStartupDir();
    if (dir.isEmpty()) return QString();
    return dir + QDir::separator() + QStringLiteral("SnipOCR.lnk");
}

bool SettingsDialog::createStartupShortcut()
{
#ifdef Q_OS_WIN
    QString lnkPath = getShortcutPath();
    if (lnkPath.isEmpty()) return false;

    QString exePath = QCoreApplication::applicationFilePath();
    QString workDir = QFileInfo(exePath).absolutePath();

    // Use IShellLink via CoCreateInstance (no external dependency)
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    Microsoft::WRL::ComPtr<IShellLink> psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&psl));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    psl->SetPath(reinterpret_cast<LPCWSTR>(exePath.utf16()));
    psl->SetWorkingDirectory(reinterpret_cast<LPCWSTR>(workDir.utf16()));
    psl->SetShowCmd(SW_SHOWMINNOACTIVE); // Start minimized to tray

    Microsoft::WRL::ComPtr<IPersistFile> ppf;
    hr = psl.As(&ppf);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    hr = ppf->Save(reinterpret_cast<LPCOLESTR>(lnkPath.utf16()), TRUE);
    CoUninitialize();
    return SUCCEEDED(hr);
#else
    return false;
#endif
}

bool SettingsDialog::removeStartupShortcut()
{
    QString lnkPath = getShortcutPath();
    if (lnkPath.isEmpty()) return true;
    QFile f(lnkPath);
    if (f.exists()) {
        return f.remove();
    }
    return true;
}

bool SettingsDialog::isAutoStartEnabled()
{
    QString lnkPath = getShortcutPath();
    if (lnkPath.isEmpty()) return false;
    return QFile::exists(lnkPath);
}

void SettingsDialog::syncAutoStart(bool enabled)
{
    if (enabled) {
        createStartupShortcut();
    } else {
        removeStartupShortcut();
    }
}

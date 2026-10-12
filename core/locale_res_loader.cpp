#include "locale_res_loader.h"
#include "resource_helper.h"
#include "crypto.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QByteArray>

namespace LocaleResLoader {

static const quint32 BIN_MAGIC = 0x4C4F4345;
static const quint32 BIN_VERSION = 0x0001;

static const QByteArray AES_KEY = QByteArray::fromHex(
    "2A7FB13C8E549D16C308F56AE14B2879"
    "52D9AFC7136F80BD9435EE07CA214F6D"
);

static QJsonObject _fallback_zh_cn() {
    return QJsonObject{
        {"app.title", "SnipOCR"},
        {"tray.snapshot", "截图"},
        {"tray.ocr", "截图OCR"},
        {"tray.delay_screenshot", "延时截图"},
        {"tray.preferences", "首选项"},
        {"tray.exit", "退出"},
        {"capture.drag_hint", "拖拽选择截图区域"},
        {"capture.copy", "复制到剪贴板"},
        {"capture.ocr", "OCR 文字识别"},
        {"capture.save", "保存图片"},
        {"capture.pin", "贴图"},
        {"capture.cancel", "取消"},
        {"settings.title", "设置"},
        {"settings.save", "保存"},
        {"settings.cancel", "取消"},
        {"common.close", "关闭"},
        {"common.later", "稍后再说"}
    };
}

static QString _locales_dir() {
    return ResourceHelper::get_resource_path("locales");
}

static bool _is_dev_mode() {
    QString mark = QDir(_locales_dir()).filePath("zh_CN.json");
    return QFile::exists(mark);
}

static QJsonObject _parse_bin_and_get(const QString& lang_id) {
    QString binPath = QDir(_locales_dir()).filePath("locales_res.bin");
    QFile f(binPath);
    if (!f.open(QIODevice::ReadOnly)) return QJsonObject();
    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 12) return QJsonObject();

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 magic, version, count;
    ds >> magic >> version >> count;
    if (magic != BIN_MAGIC || version != BIN_VERSION) return QJsonObject();

    for (quint32 i = 0; i < count; ++i) {
        quint16 nameLen;
        ds >> nameLen;
        QByteArray nameBytes = ds.device()->read(nameLen);
        QString entryLang = QString::fromUtf8(nameBytes);

        quint32 cipherLen;
        ds >> cipherLen;
        QByteArray cipher = ds.device()->read(cipherLen);
        QByteArray iv = ds.device()->read(16);

        if (entryLang == lang_id) {
            QByteArray plain = Crypto::aes_decrypt_cbc(AES_KEY, iv, cipher);
            if (plain.isEmpty()) return QJsonObject();
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(plain, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                return doc.object();
            }
            return QJsonObject();
        }
    }
    return QJsonObject();
}

static QJsonObject _parse_bin_file(const QString& binPath) {
    QFile f(binPath);
    if (!f.open(QIODevice::ReadOnly)) return QJsonObject();
    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 28) return QJsonObject();

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 magic, version;
    ds >> magic >> version;
    if (magic != BIN_MAGIC || version != BIN_VERSION) return QJsonObject();

    quint32 cipherLen;
    ds >> cipherLen;
    QByteArray cipher = ds.device()->read(cipherLen);
    QByteArray iv = ds.device()->read(16);
    if (cipher.size() != (int)cipherLen || iv.size() != 16) return QJsonObject();

    QByteArray plain = Crypto::aes_decrypt_cbc(AES_KEY, iv, cipher);
    if (plain.isEmpty()) return QJsonObject();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(plain, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        return doc.object();
    }
    return QJsonObject();
}

QJsonObject get_locale_resource(const QString& lang_id) {
    if (_is_dev_mode()) {
        QString path = QDir(_locales_dir()).filePath(lang_id + ".json");
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return QJsonObject();
        QByteArray data = f.readAll();
        f.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            return doc.object();
        }
        return QJsonObject();
    }

    // Prefer per-language encrypted file: <lang_id>.bin (Snipaste style)
    QString binPath = QDir(_locales_dir()).filePath(lang_id + ".bin");
    QJsonObject result = _parse_bin_file(binPath);
    if (!result.isEmpty()) return result;

    // Fallback: legacy bundled locales_res.bin
    result = _parse_bin_and_get(lang_id);
    if (!result.isEmpty()) return result;

    if (lang_id == "zh_CN") {
        return _fallback_zh_cn();
    }
    return QJsonObject();
}

QJsonObject get_fallback_zh_cn() {
    return _fallback_zh_cn();
}

} // namespace LocaleResLoader

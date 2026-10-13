/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "img_res_loader.h"
#include "resource_helper.h"
#include "crypto.h"
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

namespace ImgResLoader {

static const quint32 BIN_MAGIC = 0x494D4752;
static const quint32 BIN_VERSION = 0x0001;

static const QByteArray AES_KEY = QByteArray::fromHex(
    "4B1FA37C9ED40D26F348E59AC16B3869"
    "62E9BFD7237F90CDA455FE17DA315F7D"
);


static QHash<QString, QByteArray> s_bytesCache;
static QHash<QString, QPixmap>    s_pixmapCache;
static QHash<QString, QIcon>      s_iconCache;
static QMutex s_mutex;

static QString _img_dir() {
    return ResourceHelper::get_resource_path("img");
}

static bool _is_dev_mode() {
    QString mark = QDir(_img_dir()).filePath("check.svg");
    return QFile::exists(mark);
}


static void _load_all_from_bin() {
    QMutexLocker lock(&s_mutex);
    if (!s_bytesCache.isEmpty()) return; 

    QString binPath = QDir(_img_dir()).filePath("img_res.bin");
    QFile f(binPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    if (data.size() < 12) return;

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 magic, version, count;
    ds >> magic >> version >> count;
    if (magic != BIN_MAGIC || version != BIN_VERSION) return;

    for (quint32 i = 0; i < count; ++i) {
        quint16 nameLen;
        ds >> nameLen;
        QByteArray nameBytes = ds.device()->read(nameLen);
        QString entryName = QString::fromUtf8(nameBytes);

        quint32 cipherLen;
        ds >> cipherLen;
        QByteArray cipher = ds.device()->read(cipherLen);
        QByteArray iv = ds.device()->read(16);

        QByteArray plain = Crypto::aes_decrypt_cbc(AES_KEY, iv, cipher);
        if (!plain.isEmpty()) {
            s_bytesCache.insert(entryName, plain);
        }
    }
}

QByteArray get_img_bytes(const QString& filename) {
    if (_is_dev_mode()) {
        QString path = QDir(_img_dir()).filePath(filename);
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return QByteArray();
        return f.readAll();
    }
    
    _load_all_from_bin();
    QMutexLocker lock(&s_mutex);
    return s_bytesCache.value(filename);
}

QString get_img_path(const QString& filename) {
    if (_is_dev_mode()) {
        return QDir(_img_dir()).filePath(filename);
    }
    return QString();
}

QPixmap get_img_pixmap(const QString& filename) {
    {
        QMutexLocker lock(&s_mutex);
        auto it = s_pixmapCache.constFind(filename);
        if (it != s_pixmapCache.constEnd()) return it.value();
    }
    QPixmap pm;
    QByteArray data = get_img_bytes(filename);
    if (!data.isEmpty()) {
        pm.loadFromData(data);
    }
    QMutexLocker lock(&s_mutex);
    s_pixmapCache.insert(filename, pm);
    return pm;
}

QIcon get_img_icon(const QString& filename) {
    {
        QMutexLocker lock(&s_mutex);
        auto it = s_iconCache.constFind(filename);
        if (it != s_iconCache.constEnd()) return it.value();
    }
    QPixmap pm = get_img_pixmap(filename);
    QIcon icon;
    if (!pm.isNull()) icon = QIcon(pm);
    QMutexLocker lock(&s_mutex);
    s_iconCache.insert(filename, icon);
    return icon;
}

} // namespace ImgResLoader

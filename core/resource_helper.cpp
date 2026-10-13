/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "resource_helper.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QByteArray>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFileInfo>

namespace ResourceHelper {

static bool is_frozen() {
    
    
    QDir exeDir = QCoreApplication::applicationDirPath();
    return !exeDir.exists("python.exe");
}

QDir runtime_base_dir() {
    QString exePath = QCoreApplication::applicationDirPath();
    if (is_frozen()) {
        return QDir(exePath);
    }
    
    QDir d(exePath);
    
    for (int i = 0; i < 5; ++i) {
        if (d.exists("img") && d.exists("locales")) {
            return d;
        }
        if (!d.cdUp()) break;
    }
    return QDir(exePath);
}

QString get_resource_path(const QString& rel) {
    if (rel.isEmpty()) return QString();
    QDir base = runtime_base_dir();
    QString primary = base.filePath(rel);
    if (QFile::exists(primary)) return primary;
    
    if (rel.startsWith("model/") || rel.endsWith(".mtk-model")) {
        QDir proj = QCoreApplication::applicationDirPath();
        for (int i = 0; i < 5; ++i) {
            QString tryPath = proj.filePath(rel);
            if (QFile::exists(tryPath)) return tryPath;
            if (!proj.cdUp()) break;
        }
    }
    return primary;
}


static const QByteArray SP1_XOR_KEY = QByteArray::fromHex("5A3C7E9B2D4F6A8C1E3B5D7F9A2C4E6B");

QString to_sp1_name(const QString& png_name) {
    QFileInfo fi(png_name);
    return fi.completeBaseName() + ".sp1";
}

bool save_encrypted_png_bytes(const QByteArray& png_bytes, const QString& dst_path) {
    QByteArray xored = png_bytes;
    for (int i = 0; i < xored.size(); ++i) {
        xored[i] = xored[i] ^ SP1_XOR_KEY[i % SP1_XOR_KEY.size()];
    }
    QByteArray b64 = xored.toBase64();
    QFile f(dst_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(b64);
    f.close();
    return true;
}

QByteArray load_encrypted_png_bytes(const QString& sp1_path) {
    QFile f(sp1_path);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    QByteArray b64 = f.readAll();
    f.close();
    QByteArray xored = QByteArray::fromBase64(b64);
    for (int i = 0; i < xored.size(); ++i) {
        xored[i] = xored[i] ^ SP1_XOR_KEY[i % SP1_XOR_KEY.size()];
    }
    return xored;
}


static QString index_dat_path() {
    return runtime_base_dir().filePath("history/index.dat");
}

static QByteArray xor_bytes(const QByteArray& data) {
    QByteArray out = data;
    for (int i = 0; i < out.size(); ++i) {
        out[i] = out[i] ^ SP1_XOR_KEY[i % SP1_XOR_KEY.size()];
    }
    return out;
}

static QJsonObject load_index_json() {
    QFile f(index_dat_path());
    if (!f.open(QIODevice::ReadOnly)) return QJsonObject();
    QByteArray b64 = f.readAll();
    f.close();
    if (b64.isEmpty()) return QJsonObject();
    QByteArray xored = QByteArray::fromBase64(b64);
    QByteArray json = xor_bytes(xored);
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return QJsonObject();
    return doc.object();
}

static bool save_index_json(const QJsonObject& obj) {
    QDir base = runtime_base_dir();
    QDir hist = base.filePath("history");
    hist.mkpath(".");
    QJsonDocument doc(obj);
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    QByteArray xored = xor_bytes(json);
    QByteArray b64 = xored.toBase64();
    QFile f(index_dat_path());
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(b64);
    f.close();
    return true;
}

bool set_snip_entry(const QString& snipName, const QString& relPath) {
    QJsonObject idx = load_index_json();
    QJsonObject entries = idx.value("snip_entries").toObject();
    entries[snipName] = relPath;
    idx["snip_entries"] = entries;
    return save_index_json(idx);
}

QString get_snip_entry(const QString& snipName) {
    QJsonObject idx = load_index_json();
    QJsonObject entries = idx.value("snip_entries").toObject();
    return entries.value(snipName).toString();
}

bool remove_snip_entry(const QString& snipName) {
    QJsonObject idx = load_index_json();
    QJsonObject entries = idx.value("snip_entries").toObject();
    if (!entries.contains(snipName)) return false;
    entries.remove(snipName);
    idx["snip_entries"] = entries;
    return save_index_json(idx);
}

QJsonObject load_snip_index() {
    return load_index_json();
}

bool set_sticker_state(const QString& snipName, int x, int y, qreal scale,
                       int origW, int origH, qreal opacity, bool hMirror, bool vMirror) {
    QJsonObject idx = load_index_json();
    QJsonObject states = idx.value("sticker_states").toObject();
    QJsonObject st;
    st["x"] = x;
    st["y"] = y;
    st["scale"] = scale;
    st["orig_w"] = origW;
    st["orig_h"] = origH;
    st["opacity"] = opacity;
    st["h_mirror"] = hMirror;
    st["v_mirror"] = vMirror;
    states[snipName] = st;
    idx["sticker_states"] = states;
    return save_index_json(idx);
}

QJsonObject get_sticker_state(const QString& snipName) {
    QJsonObject idx = load_index_json();
    QJsonObject states = idx.value("sticker_states").toObject();
    return states.value(snipName).toObject();
}

bool remove_sticker_state(const QString& snipName) {
    QJsonObject idx = load_index_json();
    QJsonObject states = idx.value("sticker_states").toObject();
    if (!states.contains(snipName)) return false;
    states.remove(snipName);
    idx["sticker_states"] = states;
    return save_index_json(idx);
}


bool clear_all_history() {
    QDir base = runtime_base_dir();
    QDir histDir = base.filePath("history");
    if (!histDir.exists()) return true;

    // Remove all group subdirectories and their files
    QStringList subDirs = histDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : subDirs) {
        QDir subDir(histDir.filePath(sub));
        // Remove all files in subdirectory
        QStringList files = subDir.entryList(QDir::Files);
        for (const QString& f : files) {
            subDir.remove(f);
        }
        subDir.rmdir(histDir.filePath(sub));
    }

    // Clear index.dat
    QJsonObject emptyIdx;
    emptyIdx["snip_entries"] = QJsonObject();
    emptyIdx["sticker_states"] = QJsonObject();
    save_index_json(emptyIdx);

    return true;
}
} // namespace ResourceHelper

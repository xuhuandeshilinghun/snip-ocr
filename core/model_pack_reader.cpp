/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "model_pack_reader.h"
#include "crypto.h"
#include <QDataStream>
#include <QDebug>

static const char MTKM_MAGIC[4] = { 'M', 'T', 'K', 'M' };
static const uint16_t MTKM_VERSION = 0x0002;

ModelPackReader::ModelPackReader(const QString& path)
    : m_path(path), m_file(path) {}

ModelPackReader::~ModelPackReader() {
    if (m_file.isOpen()) m_file.close();
}

bool ModelPackReader::open() {
    if (m_opened) return true;
    if (!m_file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot open pack: %1").arg(m_path);
        return false;
    }
    if (!parseHeader()) return false;
    if (!parseIndex()) return false;
    m_opened = true;
    return true;
}

bool ModelPackReader::isOpen() const { return m_opened; }
QString ModelPackReader::lastError() const { return m_error; }

bool ModelPackReader::parseHeader() {
    if (m_file.size() < 16) {
        m_error = "Pack too small (< 16 bytes)";
        return false;
    }
    QByteArray hdr = m_file.read(16);
    if (hdr.left(4) != QByteArray(MTKM_MAGIC, 4)) {
        m_error = "Invalid MTKM magic";
        return false;
    }
    // big-endian: version(2), file_count(2)
    m_version = ((uint8_t)hdr[4] << 8) | (uint8_t)hdr[5];
    m_fileCount = ((uint8_t)hdr[6] << 8) | (uint8_t)hdr[7];
    if (m_version != MTKM_VERSION) {
        m_error = QString("Unsupported pack version: 0x%1 (need 0x%2)")
            .arg(m_version, 4, 16, QChar('0')).arg(MTKM_VERSION, 4, 16, QChar('0'));
        return false;
    }
    return true;
}

QByteArray ModelPackReader::decryptWithIv(const QByteArray& encData) {
    if (encData.size() < 16) return {};
    QByteArray iv = encData.left(16);
    QByteArray ciphertext = encData.mid(16);
    return Crypto::aes_decrypt_cbc(Crypto::mtkmAesKey(), iv, ciphertext);
}

bool ModelPackReader::parseIndex() {
    for (uint16_t i = 0; i < m_fileCount; ++i) {
        // name_len (uint16 BE)
        QByteArray nl = m_file.read(2);
        if (nl.size() != 2) { m_error = "Index truncated (name_len)"; return false; }
        uint16_t nameLen = ((uint8_t)nl[0] << 8) | (uint8_t)nl[1];

        // enc_name (AES: iv + ciphertext)
        QByteArray encName = m_file.read(nameLen);
        if (encName.size() != (int)nameLen) { m_error = "Index truncated (enc_name)"; return false; }
        QString name = QString::fromUtf8(decryptWithIv(encName));
        if (name.isEmpty()) name = QString::fromUtf8(encName); // fallback to raw

        // offset (uint64 BE)
        QByteArray off = m_file.read(8);
        if (off.size() != 8) { m_error = "Index truncated (offset)"; return false; }
        uint64_t offset = 0;
        for (int b = 0; b < 8; ++b) offset = (offset << 8) | (uint8_t)off[b];

        // size (uint64 BE)
        QByteArray sz = m_file.read(8);
        if (sz.size() != 8) { m_error = "Index truncated (size)"; return false; }
        uint64_t size = 0;
        for (int b = 0; b < 8; ++b) size = (size << 8) | (uint8_t)sz[b];

        // sha256 (32 bytes raw)
        QByteArray sha = m_file.read(32);
        if (sha.size() != 32) { m_error = "Index truncated (sha256)"; return false; }
        QByteArray shaHex = sha.toHex();

        if (!name.isEmpty()) {
            ResourceEntry e;
            e.name = name;
            e.offset = offset;
            e.size = size;
            e.sha256hex = shaHex;
            m_nameIndex[name] = m_entries.size();
            m_entries.append(e);
        }
    }
    return true;
}

QStringList ModelPackReader::listResources() const {
    QStringList names;
    for (const auto& e : m_entries) names.append(e.name);
    return names;
}

bool ModelPackReader::hasResource(const QString& name) const {
    return m_nameIndex.contains(name);
}

QByteArray ModelPackReader::getResource(const QString& name, bool* ok) {
    if (ok) *ok = false;
    auto it = m_nameIndex.constFind(name);
    if (it == m_nameIndex.constEnd()) {
        m_error = QString("Resource not found: %1").arg(name);
        return {};
    }
    const ResourceEntry& e = m_entries[*it];
    if (!m_file.seek((qint64)e.offset)) {
        m_error = QString("Seek failed for %1").arg(name);
        return {};
    }
    QByteArray encData = m_file.read((qint64)e.size);
    if (encData.size() != (int)e.size) {
        m_error = QString("Short read for %1").arg(name);
        return {};
    }
    QByteArray data = decryptWithIv(encData);
    if (data.isEmpty()) {
        m_error = QString("Decrypt failed for %1").arg(name);
        return {};
    }
    if (!e.sha256hex.isEmpty()) {
        QByteArray actual = Crypto::sha256Hex(data);
        if (actual != e.sha256hex) {
            m_error = QString("SHA256 mismatch for %1").arg(name);
            return {};
        }
    }
    if (ok) *ok = true;
    return data;
}

QByteArray ModelPackReader::tryGetResource(const QString& name, bool* ok) {
    QStringList variants;
    QString n1 = name;
    variants << n1 << n1.replace("/", "\\") << name.toLower() << name.toUpper();
    for (const QString& v : variants) {
        if (hasResource(v)) return getResource(v, ok);
    }
    if (ok) *ok = false;
    m_error = QString("Resource not found (variants): %1").arg(name);
    return {};
}

/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QFile>
#include <QMap>
#include <cstdint>

class ModelPackReader {
public:
    explicit ModelPackReader(const QString& path);
    ~ModelPackReader();

    bool open();
    bool isOpen() const;
    QString lastError() const;

    QStringList listResources() const;
    QByteArray getResource(const QString& name, bool* ok = nullptr);
    bool hasResource(const QString& name) const;
    QByteArray tryGetResource(const QString& name, bool* ok = nullptr);

private:
    bool parseHeader();
    bool parseIndex();
    QByteArray decryptWithIv(const QByteArray& encData);

    QString m_path;
    QFile m_file;
    bool m_opened = false;
    QString m_error;

    uint16_t m_version = 0;
    uint16_t m_fileCount = 0;

    struct ResourceEntry {
        QString name;
        uint64_t offset;
        uint64_t size;
        QByteArray sha256hex;
    };
    QVector<ResourceEntry> m_entries;
    QMap<QString, int> m_nameIndex;
};

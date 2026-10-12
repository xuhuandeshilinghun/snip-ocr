#pragma once
#include <QString>
#include <QDir>
#include <QJsonObject>

namespace ResourceHelper {
    // runtime resource root dir
    QDir runtime_base_dir();
    // resolve absolute resource path from relative
    QString get_resource_path(const QString& rel);
    // sticker encryption helpers
    QString to_sp1_name(const QString& png_name);
    bool save_encrypted_png_bytes(const QByteArray& png_bytes, const QString& dst_path);
    QByteArray load_encrypted_png_bytes(const QString& sp1_path);

    // index.dat encrypted index (XOR + base64, JSON payload)
    // snip entry: snip filename -> original path relative to history/
    bool set_snip_entry(const QString& snipName, const QString& relPath);
    QString get_snip_entry(const QString& snipName);
    bool remove_snip_entry(const QString& snipName);
    QJsonObject load_snip_index();

    // sticker desktop state persistence (pos/scale/orig size/opacity/mirror)
    bool set_sticker_state(const QString& snipName, int x, int y, qreal scale,
                           int origW, int origH, qreal opacity, bool hMirror, bool vMirror);
    QJsonObject get_sticker_state(const QString& snipName);
    bool remove_sticker_state(const QString& snipName);

    // Clear all screenshot history (files + index)
    bool clear_all_history();
}

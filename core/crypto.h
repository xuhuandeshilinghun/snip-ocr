/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QByteArray>
#include <cstdint>

namespace Crypto {
    
    QByteArray aes_encrypt_cbc(const QByteArray& key, const QByteArray& iv,
                               const QByteArray& plaintext);

    
    QByteArray aes_decrypt_cbc(const QByteArray& key, const QByteArray& iv,
                               const QByteArray& ciphertext);

    
    QByteArray sha256(const QByteArray& data);

    
    QByteArray sha256Hex(const QByteArray& data);

    
    QByteArray generate_iv();

    
    QByteArray mtkmAesKey();
}

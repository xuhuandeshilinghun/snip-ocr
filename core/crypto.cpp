/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "crypto.h"
#include <windows.h>
#include <bcrypt.h>
#include <QRandomGenerator>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace Crypto {

static bool aes_cbc(const QByteArray& key, const QByteArray& iv,
                    const QByteArray& input, QByteArray& output, bool encrypt) {
    if (key.size() != 32 || iv.size() != 16) return false;
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    bool ok = false;
    do {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (status < 0) break;
        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                                   (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                                   sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if (status < 0) break;
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                                            (PUCHAR)key.constData(), key.size(), 0);
        if (status < 0) break;
        std::vector<UCHAR> ivCopy(iv.begin(), iv.end());
        ULONG outLen = 0;
        if (encrypt) {
            status = BCryptEncrypt(hKey, (PUCHAR)input.constData(), input.size(),
                                   nullptr, ivCopy.data(), ivCopy.size(),
                                   nullptr, 0, &outLen, BCRYPT_BLOCK_PADDING);
            if (status < 0) break;
            output.resize(outLen);
            ivCopy.assign(iv.begin(), iv.end());
            status = BCryptEncrypt(hKey, (PUCHAR)input.constData(), input.size(),
                                   nullptr, ivCopy.data(), ivCopy.size(),
                                   (PUCHAR)output.data(), outLen, &outLen,
                                   BCRYPT_BLOCK_PADDING);
        } else {
            status = BCryptDecrypt(hKey, (PUCHAR)input.constData(), input.size(),
                                   nullptr, ivCopy.data(), ivCopy.size(),
                                   nullptr, 0, &outLen, BCRYPT_BLOCK_PADDING);
            if (status < 0) break;
            output.resize(outLen);
            ivCopy.assign(iv.begin(), iv.end());
            status = BCryptDecrypt(hKey, (PUCHAR)input.constData(), input.size(),
                                   nullptr, ivCopy.data(), ivCopy.size(),
                                   (PUCHAR)output.data(), outLen, &outLen,
                                   BCRYPT_BLOCK_PADDING);
        }
        if (status < 0) { output.clear(); break; }
        output.resize(outLen);
        ok = true;
    } while (false);
    if (hKey) BCryptDestroyKey(hKey);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    return ok;
}

QByteArray aes_encrypt_cbc(const QByteArray& key, const QByteArray& iv,
                           const QByteArray& plaintext) {
    QByteArray out;
    aes_cbc(key, iv, plaintext, out, true);
    return out;
}

QByteArray aes_decrypt_cbc(const QByteArray& key, const QByteArray& iv,
                           const QByteArray& ciphertext) {
    QByteArray out;
    aes_cbc(key, iv, ciphertext, out, false);
    return out;
}

QByteArray sha256(const QByteArray& data) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    QByteArray result;
    do {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        if (status < 0) break;
        status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
        if (status < 0) break;
        status = BCryptHashData(hHash, (PUCHAR)data.constData(), data.size(), 0);
        if (status < 0) break;
        result.resize(32);
        status = BCryptFinishHash(hHash, (PUCHAR)result.data(), 32, 0);
        if (status < 0) { result.clear(); break; }
    } while (false);
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

QByteArray sha256Hex(const QByteArray& data) {
    QByteArray raw = sha256(data);
    QByteArray hex;
    hex.resize(raw.size() * 2);
    static const char hexd[] = "0123456789abcdef";
    for (int i = 0; i < raw.size(); ++i) {
        uchar b = (uchar)raw[i];
        hex[i*2] = hexd[b >> 4];
        hex[i*2+1] = hexd[b & 0x0f];
    }
    return hex;
}

QByteArray generate_iv() {
    QByteArray iv(16, Qt::Uninitialized);
    for (int i = 0; i < 16; ++i) {
        iv[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFF);
    }
    return iv;
}

QByteArray mtkmAesKey() {
    static const uchar masks[4][8] = {
        {196, 110, 145, 151, 30, 6, 208, 113},
        {133, 99, 135, 165, 102, 191, 108, 179},
        {171, 28, 246, 189, 251, 132, 93, 30},
        {188, 7, 246, 27, 254, 215, 7, 156},
    };
    static const uchar frags[4][8] = {
        {176, 37, 244, 238, 45, 52, 146, 24},
        {202, 32, 213, 246, 3, 220, 30, 214},
        {230, 121, 146, 212, 154, 208, 50, 113},
        {208, 108, 159, 111, 204, 231, 53, 170},
    };
    static const int order[4] = {2, 3, 1, 0};
    QByteArray key(32, Qt::Uninitialized);
    for (int k = 0; k < 4; ++k) {
        int o = order[k];
        for (int i = 0; i < 8; ++i) {
            key[k*8 + i] = (char)(frags[o][i] ^ masks[o][i]);
        }
    }
    return key;
}

} // namespace Crypto

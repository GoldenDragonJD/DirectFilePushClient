#pragma once
#include <QByteArray>
#include <QCryptographicHash>

inline QByteArray hmacSha256(
    const QByteArray& key,
    const QByteArray& message)
{
    QByteArray k = key;
    if (k.size() > 64)
        k = QCryptographicHash::hash(k, QCryptographicHash::Sha256);

    k = k.leftJustified(64, '\0');

    QByteArray o_key_pad(64, '\x5c');
    QByteArray i_key_pad(64, '\x36');

    for (int i = 0; i < 64; ++i) {
        o_key_pad[i] ^= k[i];
        i_key_pad[i] ^= k[i];
    }

    QByteArray inner = QCryptographicHash::hash(
        i_key_pad + message,
        QCryptographicHash::Sha256
        );

    return QCryptographicHash::hash(
        o_key_pad + inner,
        QCryptographicHash::Sha256
        );
}


#pragma once
#include <QByteArray>
#include <QCryptographicHash>
#include <cstdint>

class SimpleStreamCipher {
public:
    SimpleStreamCipher(const QByteArray& key,
                       const QByteArray& nonce)
        : m_key(key),
        m_nonce(nonce),
        m_counter(0),
        m_streamOffset(0)
    {}

    void process(QByteArray& data) {
        for (int i = 0; i < data.size(); ++i) {
            if (m_streamOffset == m_stream.size()) {
                m_stream = generateKeystreamBlock();
                m_streamOffset = 0;
            }
            data[i] ^= m_stream[m_streamOffset++];
        }
    }

private:
    QByteArray m_key;
    QByteArray m_nonce;
    uint64_t   m_counter;

    QByteArray m_stream;
    int        m_streamOffset;

    QByteArray generateKeystreamBlock() {
        QByteArray input;
        input.append(m_key);
        input.append(m_nonce);
        input.append(reinterpret_cast<const char*>(&m_counter),
                     sizeof(m_counter));
        m_counter++;

        return QCryptographicHash::hash(
            input,
            QCryptographicHash::Sha256
            );
    }
};

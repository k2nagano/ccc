// sonar_sender.cc
// Compile with:
// g++ sonar_sender.cc -o sonar_sender `pkg-config --cflags --libs Qt5Network` -std=c++11

#include <QtCore/QCoreApplication>
#include <QtNetwork/QUdpSocket>
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QTimer>
#include <iostream>
#include <QThread>

constexpr quint16 DEST_PORT = 5700;
const QString DEST_IP = "127.0.0.1";
const int MAX_PAYLOAD = 60000;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: sonar_sender <mkv_or_raw_file>\n";
        return 1;
    }

    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    QUdpSocket socket;
    QHostAddress dest(DEST_IP);

    // Example settings
    const quint16 width = 256;
    const quint16 height = 100;
    const quint16 swath = 120;
    const quint16 range = 30;
    const quint32 is16bit = 0;  // 0 = 8bit, 1 = 16bit

    QByteArray frameData = file.read(width * height);  // Read 1 frame

    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    headerStream.setByteOrder(QDataStream::BigEndian);
    headerStream << width << height << swath << range << is16bit;

    QByteArray fullPayload = header + frameData;
    int totalSize = fullPayload.size();
    int chunkSize = MAX_PAYLOAD;
    int numChunks = (totalSize + chunkSize - 1) / chunkSize;

    for (int i = 0; i < numChunks; ++i) {
        QByteArray chunk;
        QDataStream ds(&chunk, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << static_cast<quint32>(i);  // Sequence number
        chunk.append(fullPayload.mid(i * chunkSize, chunkSize));
        socket.writeDatagram(chunk, dest, DEST_PORT);
        QThread::msleep(1);  // Optional pacing
    }

    std::cout << "Sent " << numChunks << " UDP packets." << std::endl;
    return 0;
}


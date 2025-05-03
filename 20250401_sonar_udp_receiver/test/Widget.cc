#include "Widget.hh"
#include <QtCore/QDataStream>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>
#include <cmath>

// UdpWorker: CHUNKED UDP を受信し、再構築して Widget に通知する
class UdpWorker : public QObject
{
public:
    UdpWorker(quint16 port, int maxPayload, Widget* target)
        : m_port(port), m_maxPayload(maxPayload), m_target(target)
    {
        sock.bind(QHostAddress::LocalHost, m_port, QUdpSocket::ShareAddress);
        connect(&sock, &QUdpSocket::readyRead, this, &UdpWorker::processDatagrams,
                Qt::DirectConnection);
    }

private:
    void
    processDatagrams()
    {
        while (sock.hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(int(sock.pendingDatagramSize()));
            sock.readDatagram(datagram.data(), datagram.size());
            if (datagram.size() < 4)
                continue;

            QDataStream seqStream(datagram);
            seqStream.setByteOrder(QDataStream::BigEndian);
            quint32 seq;
            seqStream >> seq;
            QByteArray chunk = datagram.mid(4);

            if (seq == 0)
            {
                buffer.clear();
                headerParsed = false;
                expectedSize = 0;
            }
            int offset = int(seq) * m_maxPayload;
            int need = offset + chunk.size();
            if (buffer.size() < need)
                buffer.resize(need);
            memcpy(buffer.data() + offset, chunk.constData(), chunk.size());

            if (!headerParsed && buffer.size() >= 12)
            {
                QDataStream hdr(buffer);
                hdr.setByteOrder(QDataStream::BigEndian);
                quint16 w, h, sw, rg;
                quint32 is16;
                hdr >> w >> h >> sw >> rg >> is16;
                int pixelBytes = int(w) * int(h) * (is16 ? 2 : 1);
                expectedSize = 12 + pixelBytes;
                headerParsed = true;
                curW = w;
                curH = h;
                curSw = sw;
                curRg = rg;
                curIs16 = (is16 == 1);
            }
            if (headerParsed && buffer.size() >= expectedSize)
            {
                QByteArray full = buffer.left(expectedSize);
                QByteArray raw = full.mid(12);
                // UI スレッドに通知
                QMetaObject::invokeMethod(m_target, "onFrameDecoded", Qt::QueuedConnection,
                                          Q_ARG(int, curW), Q_ARG(int, curH), Q_ARG(int, curSw),
                                          Q_ARG(int, curRg), Q_ARG(bool, curIs16),
                                          Q_ARG(QByteArray, raw));
                headerParsed = false;
            }
        }
    }

    quint16 m_port;
    int m_maxPayload;
    Widget* m_target;
    QUdpSocket sock;
    QByteArray buffer;
    int expectedSize{0};
    bool headerParsed{false};
    int curW{0}, curH{0}, curSw{0}, curRg{0};
    bool curIs16{false};
};

// explicit
Widget::Widget(QWidget* pParent) : QWidget(pParent)
{
    m_worker = new UdpWorker(5700, 60000, this);
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();
    setMinimumSize(600, 600);
}

Widget::~Widget()
{
    m_thread->quit();
    m_thread->wait();
}

void
Widget::onFrameDecoded(int w, int h, int sw, int rg, bool is16, QByteArray raw)
{
    m_width = w;
    m_height = h;
    m_swath = sw;
    m_range = rg;
    m_is16bit = is16;
    int count = w * h;
    if (is16)
    {
        m_frame16.resize(count);
        memcpy(m_frame16.data(), raw.constData(), count * sizeof(quint16));
    }
    else
    {
        m_frame8.resize(count);
        memcpy(m_frame8.data(), raw.constData(), count * sizeof(quint8));
    }
    update();
}

void
Widget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), mBackgroundColor);
    if ((m_is16bit && m_frame16.isEmpty()) || (!m_is16bit && m_frame8.isEmpty()))
        return;

    QPointF center(width() / 2.0, height());
    double scale = (height() * 0.9) / m_range;
    painter.setPen(mForegroundColor);

    int tick = calculateTickStep(m_range);
    for (int d = tick; d <= m_range; d += tick)
    {
        double r = d * scale;
        QRectF arc(center.x() - r, center.y() - r, 2 * r, 2 * r);
        painter.drawArc(arc, (90 + m_swath / 2) * 16, -m_swath * 16);
        painter.drawText(QPointF(center.x() + 5, center.y() - r), QString::number(d) + "m");
    }
    for (int s : {-1, 1})
    {
        double ang = M_PI / 180.0 * (s * m_swath / 2.0);
        double x = m_range * scale * std::sin(ang);
        double y = m_range * scale * std::cos(ang);
        painter.drawLine(center, QPointF(center.x() + x, center.y() - y));
    }
    for (int i = 0; i < m_width; ++i)
    {
        double ang = M_PI / 180.0 * (-m_swath / 2.0 + i * (double(m_swath) / (m_width - 1)));
        for (int r = 0; r < m_height; ++r)
        {
            int idx = r * m_width + i;
            int v = m_is16bit ? m_frame16[idx] : m_frame8[idx];
            if (v < mMinIntensity)
                v = mMinIntensity;
            if (v > mMaxIntensity)
                v = mMaxIntensity;
            double t = (mMaxIntensity > mMinIntensity)
                           ? double(v - mMinIntensity) / (mMaxIntensity - mMinIntensity)
                           : 0.0;
            QColor c =
                QColor(mMinIntensityColor.red() * (1.0 - t) + mMaxIntensityColor.red() * t,
                       mMinIntensityColor.green() * (1.0 - t) + mMaxIntensityColor.green() * t,
                       mMinIntensityColor.blue() * (1.0 - t) + mMaxIntensityColor.blue() * t);
            painter.setPen(c);
            double rr = r * (m_range / double(m_height - 1)) * scale;
            painter.drawPoint(
                QPointF(center.x() + rr * std::sin(ang), center.y() - rr * std::cos(ang)));
        }
    }
}

int
Widget::calculateTickStep(int range) const
{
    if (range <= 5)
        return 1;
    else if (range <= 10)
        return 2;
    else if (range <= 100)
        return 10;
    else
        return 50;
}
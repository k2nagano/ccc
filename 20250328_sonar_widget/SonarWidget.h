#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QImage>
#include <QColor>
#include <QPoint>
#include <QRectF>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class SonarWidget : public QWidget
{
    Q_OBJECT

public:
    SonarWidget(QWidget *parent = nullptr)
        : QWidget(parent),
          nRange(1024),
          nBeam(512),
          nSwat(120),
          bitDepth(8),
          minIntensity(0),
          maxIntensity(255),
          minColor(255, 255, 0),  // yellow
          maxColor(0, 0, 0)       // black
    {
        setMinimumSize(600, 600);
        image = QImage(size(), QImage::Format_RGB32);
        image.fill(Qt::black);

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SonarWidget::updateData);
        timer->start(100);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.drawImage(0, 0, image);

        QPoint center = rect().center();
        float radiusMax = std::min(width(), height()) / 2.0f;
        float scale = radiusMax / nRange;
        int step = nRange / 5;

        // 放射状の角度線（背景→前景）
        for (int i = 0; i < 5; ++i)
        {
            float angle = -nSwat / 2.0f + i * (nSwat / 4.0f);
            float rad = qDegreesToRadians(angle);
            float x = center.x() + radiusMax * std::sin(rad);
            float y = center.y() - radiusMax * std::cos(rad);

            painter.setPen(QColor(128, 128, 128));
            painter.drawLine(center, QPoint(x, y));
            painter.setPen(Qt::white);
            painter.drawLine(center, QPoint(x, y));
        }

        // 同心円の距離目盛（背景→前景）
        for (int r = step; r <= nRange; r += step)
        {
            float radius = r * scale;
            QRectF arcRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
            int startAngle = static_cast<int>((-nSwat / 2 + 90) * 16);
            int spanAngle = static_cast<int>(nSwat * 16);

            painter.setPen(QColor(128, 128, 128));
            painter.drawArc(arcRect, startAngle, spanAngle);
            painter.setPen(Qt::white);
            painter.drawArc(arcRect, startAngle, spanAngle);

            for (float a : {-nSwat / 2.0f, nSwat / 2.0f})
            {
                float rad = qDegreesToRadians(a);
                float x = center.x() + radius * std::sin(rad);
                float y = center.y() - radius * std::cos(rad);
                painter.drawText(x, y, QString("%1m").arg(r));
            }
        }
    }

private slots:
    void updateData()
    {
        std::vector<std::vector<uint16_t>> data(nBeam, std::vector<uint16_t>(nRange));
        generateDummyData(data);
        updateImage(data);
        update();
    }

private:
    int nRange, nBeam, nSwat;
    int bitDepth;
    int minIntensity, maxIntensity;
    QColor minColor, maxColor;
    QImage image;
    QTimer *timer;

    void generateDummyData(std::vector<std::vector<uint16_t>> &data)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, bitDepth == 8 ? 255 : 65535);
        for (int i = 0; i < nBeam; ++i)
            for (int j = 0; j < nRange; ++j)
                data[i][j] = dis(gen);
    }

    QColor interpolateColor(int val)
    {
        float ratio = (val - minIntensity) / float(std::max(1, maxIntensity - minIntensity));
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        int r = static_cast<int>(minColor.red() * (1 - ratio) + maxColor.red() * ratio);
        int g = static_cast<int>(minColor.green() * (1 - ratio) + maxColor.green() * ratio);
        int b = static_cast<int>(minColor.blue() * (1 - ratio) + maxColor.blue() * ratio);
        return QColor(r, g, b);
    }

    void updateImage(const std::vector<std::vector<uint16_t>> &data)
    {
        image.fill(Qt::black);
        QPainter painter(&image);
        QPoint center = rect().center();
        float scale = std::min(width(), height()) / 2.0f / nRange;

        float angleStart = -nSwat / 2.0f;
        float angleStep = float(nSwat) / nBeam;

        for (int i = 0; i < nBeam; ++i)
        {
            float angleDeg = angleStart + i * angleStep;
            float angleRad = qDegreesToRadians(angleDeg);

            for (int j = 0; j < nRange; ++j)
            {
                int strength = data[i][j];
                if (bitDepth == 16)
                    strength = strength * 255 / 65535;

                QColor color = interpolateColor(strength);
                float r = j * scale;
                int x = center.x() + r * std::sin(angleRad);
                int y = center.y() - r * std::cos(angleRad);
                if (image.rect().contains(x, y))
                    image.setPixelColor(x, y, color);
            }
        }
    }
};


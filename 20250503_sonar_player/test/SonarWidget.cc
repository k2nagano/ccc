#include "SonarWidget.hh"
#include <QPainter>
#include <QtMath>

// explicit
SonarWidget::SonarWidget(QWidget* pParent) : QWidget(pParent)
{
    mSwath = 120.0;
    mRange = 30.0;
    mMinIntensity = 0;
    mMaxIntensity = 255;
    mMinIntensityColor = Qt::yellow;
    mMaxIntensityColor = Qt::red;
    mForegroundColor = Qt::green;
    mBackgroundColor = Qt::black;
}

// virtual
SonarWidget::~SonarWidget()
{
}

void
SonarWidget::setFrame(const QImage& frame)
{
    mFrame = frame;
    update();
}

void
SonarWidget::setSwath(float swath)
{
    mSwath = swath;
    update();
}

void
SonarWidget::setRange(float range)
{
    mRange = range;
    update();
}

void
SonarWidget::paintEvent(QPaintEvent*)
{
    if (mFrame.isNull())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 上部余白 20px
    const int marginTop = 20;
    painter.fillRect(rect(), mBackgroundColor);
    painter.translate(0, marginTop);

    QPointF center(width() * 0.5, (height() - marginTop));
    int radius = qMin(width(), height() - marginTop);

    for (int x = 0; x < mFrame.width(); ++x)
    {
        for (int y = 0; y < mFrame.height(); ++y)
        {
            int intensity = mFrame.pixelColor(x, y).red();
            if (intensity < mMinIntensity)
            {
                intensity = mMinIntensity;
            }
            if (intensity > mMaxIntensity)
            {
                intensity = mMaxIntensity;
            }
            float t =
                (float)(intensity - mMinIntensity) / std::max(1, mMaxIntensity - mMinIntensity);
            QColor color = QColor::fromRgbF(
                mMinIntensityColor.redF() * (1 - t) + mMaxIntensityColor.redF() * t,
                mMinIntensityColor.greenF() * (1 - t) + mMaxIntensityColor.greenF() * t,
                mMinIntensityColor.blueF() * (1 - t) + mMaxIntensityColor.blueF() * t);
            // QColor color = mFrame.pixelColor(x, y);
            float angle = ((float)x / mFrame.width()) * mSwath - mSwath / 2; // -60〜+60度
            float dist = ((float)y / mFrame.height()) * radius;

            float rad = qDegreesToRadians(angle);
            QPointF point = center + QPointF(dist * qSin(rad), -dist * qCos(rad));
            painter.setPen(color);
            painter.drawPoint(point);
        }
    }

    // ────────────── 目盛り付き円弧 ──────────────
    // 間隔をレンジに応じて選択
    int step;
    if (mRange <= 5.0f)
        step = 1;
    else if (mRange <= 10.0f)
        step = 2;
    else if (mRange <= 100.0f)
        step = 10;
    else
        step = 50;
    painter.setPen(mForegroundColor);
    for (int d = step; d <= int(mRange); d += step)
    {
        float dist = float(d) / mRange * radius;
        QRectF arcRect(center.x() - dist, center.y() - dist, dist * 2, dist * 2);
        // Qt::drawArc は 1/16 度単位、反時計回りなので符号反転
        painter.drawArc(arcRect, int((90 + mSwath / 2) * 16), -int(mSwath * 16));
        // 両端に数値
        for (float angleSign : {-mSwath / 2.0f, mSwath / 2.0f})
        {
            float rad = qDegreesToRadians(angleSign);
            QPointF pt = center + QPointF(dist * qSin(rad), -dist * qCos(rad));
            painter.drawText(pt, QString::number(d));
        }
    }

    // ────────────── テキスト描画 ──────────────
    QFontMetrics fm(painter.font());
    QString txt1 = QString("Min Intensity=%1").arg(mMinIntensity);
    QString txt2 = QString("Max Intensity=%1").arg(mMaxIntensity);
    int w = qMax(fm.horizontalAdvance(txt1), fm.horizontalAdvance(txt2));
    int x = width() - w - 5;
    painter.drawText(x, fm.ascent(), txt1);
    painter.drawText(x, fm.height() + fm.ascent(), txt2);
}

QSize
SonarWidget::sizeHint() const
{
    return QSize(1000, 700); // 初期サイズを指定
}

void
SonarWidget::setMinIntensity(int minIntensity)
{
    mMinIntensity = minIntensity;
    update();
}
void
SonarWidget::setMaxIntensity(int maxIntensity)
{
    mMaxIntensity = maxIntensity;
    update();
}
void
SonarWidget::setMinIntensityColor(QColor minIntensityColor)
{
    mMinIntensityColor = minIntensityColor;
    update();
}
void
SonarWidget::setMaxIntensityColor(QColor maxIntensityColor)
{
    mMaxIntensityColor = maxIntensityColor;
    update();
}
void
SonarWidget::setForegroundColor(QColor foregroundColor)
{
    mForegroundColor = foregroundColor;
    update();
}
void
SonarWidget::setBackgroundColor(QColor backgroundColor)
{
    mBackgroundColor = backgroundColor;
    update();
}
void
SonarWidget::setElapsedDuratuion(std::chrono::milliseconds ms)
{
}
float
SonarWidget::swath()
{
    return mSwath;
}
float
SonarWidget::range()
{
    return mRange;
}
int
SonarWidget::minIntensity()
{
    return mMinIntensity;
}
int
SonarWidget::maxIntensity()
{
    return mMaxIntensity;
}
QColor
SonarWidget::minIntensityColor()
{
    return mMinIntensityColor;
}
QColor
SonarWidget::maxIntensityColor()
{
    return mMaxIntensityColor;
}
QColor
SonarWidget::foregroundColor()
{
    return mForegroundColor;
}
QColor
SonarWidget::backgroundColor()
{
    return mBackgroundColor;
}

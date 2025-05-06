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

    QPointF center(width() / 2, height());
    int radius = qMin(width(), height());

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

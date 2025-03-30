#include "SonarWidget.hh"
#include <QPainter>
#include <QtMath>

SonarWidget::SonarWidget(QWidget* parent) : QWidget(parent)
{
    mFgColor = Qt::green;
    mBgColor = Qt::black;
}

void
SonarWidget::setFrame(const QImage& frame)
{
    mCurrentFrame = frame;
    update();
}

void
SonarWidget::setRange(float r)
{
    mRange = r;
    update();
}

void
SonarWidget::paintEvent(QPaintEvent*)
{
    if (mCurrentFrame.isNull())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPointF center(width() / 2, height());
    int radius = qMin(width(), height());

    for (int x = 0; x < mCurrentFrame.width(); ++x)
    {
        for (int y = 0; y < mCurrentFrame.height(); ++y)
        {
            int intensity = mCurrentFrame.pixelColor(x, y).red();
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
            QColor color = QColor::fromRgbF(mMinColor.redF() * (1 - t) + mMaxColor.redF() * t,
                                            mMinColor.greenF() * (1 - t) + mMaxColor.greenF() * t,
                                            mMinColor.blueF() * (1 - t) + mMaxColor.blueF() * t);
            // QColor color = mCurrentFrame.pixelColor(x, y);
            float angle = ((float)x / mCurrentFrame.width()) * 120.0 - 60.0; // -60〜+60度
            float dist = ((float)y / mCurrentFrame.height()) * radius;

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
SonarWidget::setMinColor(QColor minColor)
{
    mMinColor = minColor;
    update();
}
void
SonarWidget::setMaxColor(QColor maxColor)
{
    mMaxColor = maxColor;
    update();
}
void
SonarWidget::setFgColor(QColor fgColor)
{
    mFgColor = fgColor;
    update();
}
void
SonarWidget::setBgColor(QColor bgColor)
{
    mBgColor = bgColor;
    update();
}
void
SonarWidget::setElapsedDuratuion(std::chrono::milliseconds ms)
{
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
SonarWidget::minColor()
{
    return mMinColor;
}
QColor
SonarWidget::maxColor()
{
    return mMaxColor;
}
QColor
SonarWidget::fgColor()
{
    return mFgColor;
}
QColor
SonarWidget::bgColor()
{
    return mBgColor;
}

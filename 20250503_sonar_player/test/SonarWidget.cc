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
    mForegroundColor = Qt::black;
    mBackgroundColor = Qt::white;
    mTimestampMillis = 0;
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

    painter.fillRect(rect(), mBackgroundColor);

    // 上下マージン設定
    const int topMargin = 20;
    const int bottomMargin = 10;
    const int availableHeight = height() - topMargin - bottomMargin;

    // 半扇形の最大高さ = rangeに対応する高さ（ピクセル）
    float pixelsPerMeter = availableHeight / mRange;

    // 扇形の中心を画面の下（margin含めて）に置く
    QPointF center(width() / 2.0, height() - bottomMargin);

    // ソナー点描画
    for (int y = 0; y < mFrame.height(); ++y)
    {
        float r = ((float)y / mFrame.height()) * mRange;
        float dist = r * pixelsPerMeter;

        for (int x = 0; x < mFrame.width(); ++x)
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

            float t = float(intensity - mMinIntensity) / std::max(1, mMaxIntensity - mMinIntensity);
            float angle = ((float)x / mFrame.width()) * mSwath - (mSwath / 2.0f);
            float rad = qDegreesToRadians(angle);
            QPointF point = center + QPointF(dist * qSin(rad), -dist * qCos(rad));

            QColor c = QColor::fromRgbF(
                mMinIntensityColor.redF() * (1 - t) + mMaxIntensityColor.redF() * t,
                mMinIntensityColor.greenF() * (1 - t) + mMaxIntensityColor.greenF() * t,
                mMinIntensityColor.blueF() * (1 - t) + mMaxIntensityColor.blueF() * t);

            painter.setPen(c);
            painter.drawPoint(point);
        }
    }

    // ------------------------------
    // 扇形のガイド描画
    // ------------------------------
    painter.setPen(mForegroundColor);

    // 距離円弧（間隔を mRange に応じて動的に決定）
    float step = 1.0f;
    if (mRange <= 5.0f)
        step = 1.0f;
    else if (mRange <= 10.0f)
        step = 2.0f;
    else if (mRange <= 100.0f)
        step = 10.0f;
    else
        step = 50.0f;

    // 最大ラベル数を制限（安全対策）
    int maxSteps = static_cast<int>(mRange / step);
    maxSteps = qMin(maxSteps, 100);

    for (int i = 1; i <= maxSteps; ++i)
    {
        float r_m = i * step;
        float radius = r_m * pixelsPerMeter;
        QRectF arcRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

        // 円弧を描画（QPainter::drawArc）
        painter.drawArc(arcRect, (90 - mSwath / 2) * 16, mSwath * 16);

        // ラベル描画（左右両端）
        float angleLeft = qDegreesToRadians(-mSwath / 2);
        float angleRight = qDegreesToRadians(mSwath / 2);

        QPointF left = center + QPointF(radius * qSin(angleLeft), -radius * qCos(angleLeft));
        QPointF right = center + QPointF(radius * qSin(angleRight), -radius * qCos(angleRight));

        QString label = QString("%1m").arg(r_m);
        QFontMetrics fm(painter.font());
        painter.drawText(left - QPointF(fm.horizontalAdvance(label), 0), label);
        painter.drawText(right + QPointF(2, 0), label);
    }

    // 放射角線（-60°, -30°, 0°, 30°, 60°）
    for (int i = 0; i <= 4; ++i)
    {
        float angle = -mSwath / 2 + (mSwath / 4.0f) * i;
        float rad = qDegreesToRadians(angle);
        QPointF end = center + QPointF(mRange * pixelsPerMeter * qSin(rad),
                                       -mRange * pixelsPerMeter * qCos(rad));
        painter.drawLine(center, end);
    }

    // フォントメトリクス取得
    QFontMetrics fm(painter.font());

    //  1. 左上：日時
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(mTimestampMillis);
    QString timeText = dt.toString("yyyy/MM/dd HH:mm:ss.zzz");
    painter.drawText(5, fm.ascent() + 5, timeText);

    //  2. 上中央：Range
    QString rangeLabel = QString("Range = %1 m").arg(mRange);
    int rangeTextWidth = fm.horizontalAdvance(rangeLabel);
    painter.drawText(width() / 2 - rangeTextWidth / 2, 20, rangeLabel);

    //  3. 右上：強度範囲（右寄せ・2行）
    QString minText = QString("Min Intensity=%1").arg(mMinIntensity);
    QString maxText = QString("Max Intensity=%1").arg(mMaxIntensity);
    int maxWidth = qMax(fm.horizontalAdvance(minText), fm.horizontalAdvance(maxText));

    int rightX = width() - maxWidth - 5;
    int topY = fm.ascent() + 5;
    painter.drawText(rightX, topY, minText);
    painter.drawText(rightX, topY + fm.height(), maxText);
}

// virtual
QSize
SonarWidget::sizeHint() const
{
    return QSize(720, 480); // 初期サイズを指定
}
// virtual
QSize
SonarWidget::minimumSizeHint() const
{
    return QSize(720, 480);
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

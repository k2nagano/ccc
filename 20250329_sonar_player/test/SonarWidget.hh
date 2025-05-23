#pragma once
#include <QColor>
#include <QImage>
#include <QWidget>
#include <chrono>

class SonarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SonarWidget(QWidget* parent = nullptr);
    void setFrame(const QImage& frame);
    void setSwath(float swath);
    void setRange(float range);
    virtual QSize sizeHint() const;
    void setMinIntensity(int minIntensity);
    void setMaxIntensity(int maxIntensity);
    void setMinIntensityColor(QColor minIntensityColor);
    void setMaxIntensityColor(QColor maxIntensityColor);
    void setForegroundColor(QColor foregroundColor);
    void setBackgroundColor(QColor backgroundColor);
    void setElapsedDuratuion(std::chrono::milliseconds ms);
    float swath();
    float range();
    int minIntensity();
    int maxIntensity();
    QColor minIntensityColor();
    QColor maxIntensityColor();
    QColor foregroundColor();
    QColor backgroundColor();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QImage mFrame;
    float mSwath;
    float mRange;
    int mMinIntensity;
    int mMaxIntensity;
    QColor mMinIntensityColor;
    QColor mMaxIntensityColor;
    QColor mForegroundColor;
    QColor mBackgroundColor;
};

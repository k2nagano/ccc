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
    void setRange(float r);
    virtual QSize sizeHint() const;
    void setMinIntensity(int minIntensity);
    void setMaxIntensity(int maxIntensity);
    void setMinColor(QColor minColor);
    void setMaxColor(QColor maxColor);
    void setFgColor(QColor fgColor);
    void setBgColor(QColor bgColor);
    void setElapsedDuratuion(std::chrono::milliseconds ms);
    float range();
    int minIntensity();
    int maxIntensity();
    QColor minColor();
    QColor maxColor();
    QColor fgColor();
    QColor bgColor();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QImage mCurrentFrame;
    float mRange = 10.0;
    int mMinIntensity;
    int mMaxIntensity;
    QColor mMinColor;
    QColor mMaxColor;
    QColor mFgColor;
    QColor mBgColor;
};

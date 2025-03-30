#if !defined(WIDGET_HH)
#define WIDGET_HH

#include <QtWidgets>

class Widget : public QWidget
{
public:
    explicit Widget(QWidget* pParent = nullptr);
    virtual ~Widget();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    unsigned char* mpFrame;
    int mFrameWidth;
    int mFrameHeight;
    float mRange;
    float mSwat;
    int mMinIntensity;
    int mMaxIntensity;
    QColor mFgColor;
    QColor mBgColor;
    QColor mMinColor;
    QColor mMaxColor;
    qint64 mTimestampMillis;
};

#endif // !defined(WIDGET_HH)

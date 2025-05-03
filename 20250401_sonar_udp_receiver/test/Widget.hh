#if !defined(WIDGET_HH)
#define WIDGET_HH

#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QPaintEvent;
class UdpWorker;
class QThread;

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget* pParent = nullptr);
    virtual ~Widget();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onFrameDecoded(int w, int h, int sw, int rg, bool is16, QByteArray raw);

private:
    int calculateTickStep(int range) const;

    // Sonar parameters
    int m_width{0}, m_height{0}, m_swath{0}, m_range{0};
    bool m_is16bit{false};
    QVector<quint8> m_frame8;
    QVector<quint16> m_frame16;

    // Rendering parameters
    int mMinIntensity{0};
    int mMaxIntensity{255};
    QColor mForegroundColor{Qt::white};
    QColor mBackgroundColor{Qt::black};
    QColor mMinIntensityColor{Qt::yellow};
    QColor mMaxIntensityColor{Qt::black};

    // Worker thread
    QThread* m_thread{nullptr};
    UdpWorker* m_worker{nullptr};
};

#endif // !defined(WIDGET_HH)

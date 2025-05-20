#if !defined(SONAR_PLAYER_HH)
#define SONAR_PLAYER_HH

#include "SonarThread.hh"
#include "SonarWidget.hh"
#include <QtWidgets>

class SonarPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit SonarPlayer(const QString& mkvPath, double swath = 120.0, double range = 30.0,
                         int minIntensity = 0, int maxIntensity = 255, QWidget* pParent = nullptr);
    virtual ~SonarPlayer();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

protected:
    // キー操作を受け取る
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void play();
    void pause();
    void stop();
    void fastForward();
    void rewind();
    void updateFrame(const QImage& frame);
    void setFramePosition(int pos);
    void handleFileChanged(const QString& newPath);
    void handlePlaybackStopped(int frameIndex);

private:
    SonarThread* mpSonarThread;
    SonarWidget* mpSonarWidget;

private:
    QPushButton* mpPushButtonPlay;
    QPushButton* mpPushButtonPause;
    QPushButton* mpPushButtonStop;
    QPushButton* mpPushButtonFastForward;
    QPushButton* mpPushButtonRewind;
    QDoubleSpinBox* mpDoubleSpinBoxSwath;
    QDoubleSpinBox* mpDoubleSpinBoxRange;
    QSpinBox* mpSpinBoxMinIntensity;
    QSpinBox* mpSpinBoxMaxIntensity;
    QPushButton* mpPushButtonMinIntensityColor;
    QPushButton* mpPushButtonMaxIntensityColor;
    QPushButton* mpPushButtonForegroundColor;
    QPushButton* mpPushButtonBackgroundColor;
    QSlider* mpSliderFramePosition;
    QLineEdit* mpLineEditMkvPath;

private:
    double mSwath;
    double mRange;
    int mMinIntensity;
    int mMaxIntensity;
    QColor mForegroundColor;
    QColor mBackgroundColor;
    QColor mMinIntensityColor;
    QColor mMaxIntensityColor;

private:
    // Space→Play/Pause 保持用
    enum class Mode
    {
        Play,
        Pause
    };
    Mode mMaintainedMode = Mode::Play;
};

#endif // #if !defined(SONAR_PLAYER_HH)

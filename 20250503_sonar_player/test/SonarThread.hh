#if !defined(SONAR_THREAD_HH)
#define SONAR_THREAD_HH

#include <QColorDialog>
#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>
#include <opencv2/opencv.hpp>

class SonarThread : public QThread
{
    Q_OBJECT
public:
    enum class PlaybackState
    {
        Stop,
        Play,
        Pause,
        FastForward,
        Rewind
    };
public:
    explicit SonarThread(QObject* pParent = nullptr);
    virtual ~SonarThread();

    void run() override;

    void setParams(int minI, int maxI);
    void setFilePath(const QString& path);

    void play();
    void pause();
    void stop();
    void fastForward();
    void rewind();

    int totalFrameCount() const;
    int currentFrameIndex() const;
    std::chrono::milliseconds elapsedDuration() const;
    QString currentFilePath() const;

    void setFramePosition(int pos);
    void terminate();

signals:
    void frameReady(const QImage& frame);
    void fileChanged(const QString& newPath);
    void playbackStopped(int frameIndex);

private:
    mutable QMutex mMutex;
    QString mFilePath;
    int mMinIntensity;
    int mMaxIntensity;

    cv::VideoCapture mCapture;
    int mFrameIndex;
    int mTotalFrames;
    double mFps;

    PlaybackState mState;
    bool mIsRunning;
    bool mIsPending;
    QString mPendingFilePath;
};

#endif // #if !defined(SONAR_THREAD_HH)

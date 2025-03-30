#pragma once

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

    explicit SonarThread(QObject* parent = nullptr);
    void run() override;

    void setParams(float range, int minI, int maxI);
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
    void frame_ready(const QImage& frame);
    void file_changed(const QString& newPath);
    void playbackStopped(int frameIndex);

private:
    mutable QMutex mutex;
    QString filePath;
    float range;
    int min_intensity;
    int max_intensity;

    cv::VideoCapture cap;
    int frameIndex = 0;
    int totalFrames = 0;
    double fps = 30.0;

    PlaybackState state = PlaybackState::Stop;
    bool running = true;
    bool reloadRequested = false;
    QString pendingFilePath;
};

#include "SonarThread.hh"

SonarThread::SonarThread(QObject* parent) : QThread(parent)
{
}

void
SonarThread::run()
{
    running = true;
    unsigned long sleep_msec = 1000;
    while (running && !isInterruptionRequested())
    {
        {
            QMutexLocker locker(&mutex);
            if (reloadRequested && !pendingFilePath.isEmpty())
            {
                cap.release();
                cap.open(pendingFilePath.toStdString());
                if (cap.isOpened())
                {
                    fps = cap.get(cv::CAP_PROP_FPS);
                    totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
                    frameIndex = 0;
                    filePath = pendingFilePath;
                    reloadRequested = false;
                    state = PlaybackState::Play;
                    emit file_changed(filePath);
                }
            }
        }

        {
            QMutexLocker locker(&mutex);
            if (state == PlaybackState::Play || state == PlaybackState::FastForward ||
                state == PlaybackState::Rewind)
            {
                if ((state == PlaybackState::Rewind && frameIndex <= 0) ||
                    ((state == PlaybackState::Play || state == PlaybackState::FastForward) &&
                     frameIndex >= (totalFrames - 1)))
                {
                    state = PlaybackState::Stop;
                    emit playbackStopped(frameIndex);
                }

                if (state != PlaybackState::Stop)
                {
                    int offset = 0;
                    if (state == PlaybackState::Play)
                        offset = 1;
                    else if (state == PlaybackState::FastForward)
                        offset = 30;
                    else if (state == PlaybackState::Rewind)
                        offset = -30;

                    int next = frameIndex + offset;

                    if (next < 0)
                        next = 0;
                    if (next >= totalFrames)
                        next = totalFrames - 1;

                    cap.set(cv::CAP_PROP_POS_FRAMES, next);
                    frameIndex = next;

                    cv::Mat frame;
                    if (cap.read(frame))
                    {
                        if (frame.channels() == 1)
                            cv::cvtColor(frame, frame, cv::COLOR_GRAY2RGB);

                        frame.setTo(min_intensity, frame < min_intensity);
                        frame.setTo(max_intensity, frame > max_intensity);

                        QImage image(frame.data, frame.cols, frame.rows, frame.step,
                                     QImage::Format_RGB888);
                        emit frame_ready(image.copy());
                        sleep_msec = static_cast<unsigned long>(1000.0 / fps);
                    }
                }
            }
        }
        msleep(sleep_msec);
    }
    cap.release();
}

void
SonarThread::terminate()
{
    QMutexLocker locker(&mutex);
    running = false;
}

void
SonarThread::setParams(float r, int minI, int maxI)
{
    QMutexLocker locker(&mutex);
    range = r;
    min_intensity = minI;
    max_intensity = maxI;
}

void
SonarThread::setFilePath(const QString& path)
{
    QMutexLocker locker(&mutex);
    pendingFilePath = path;
    reloadRequested = true;
}

void
SonarThread::setFramePosition(int index)
{
    QMutexLocker locker(&mutex);
    cap.set(cv::CAP_PROP_POS_FRAMES, index);
}

void
SonarThread::play()
{
    QMutexLocker locker(&mutex);
    if (state == PlaybackState::Stop)
    {
        frameIndex = 0;
        cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    }
    state = PlaybackState::Play;
}

void
SonarThread::pause()
{
    QMutexLocker locker(&mutex);
    state = PlaybackState::Pause;
}

void
SonarThread::stop()
{
    QMutexLocker locker(&mutex);
    state = PlaybackState::Stop;
}

void
SonarThread::fastForward()
{
    QMutexLocker locker(&mutex);
    state = PlaybackState::FastForward;
}

void
SonarThread::rewind()
{
    QMutexLocker locker(&mutex);
    state = PlaybackState::Rewind;
}

int
SonarThread::totalFrameCount() const
{
    QMutexLocker locker(&mutex);
    return totalFrames;
}

int
SonarThread::currentFrameIndex() const
{
    QMutexLocker locker(&mutex);
    return frameIndex;
}

QString
SonarThread::currentFilePath() const
{
    QMutexLocker locker(&mutex);
    return filePath;
}

std::chrono::milliseconds
SonarThread::elapsedDuration() const
{
    QMutexLocker locker(&mutex);
    double seconds = frameIndex / fps;
    return std::chrono::milliseconds(static_cast<int64_t>(seconds * 1000));
}

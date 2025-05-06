#include "SonarThread.hh"

// explicit
SonarThread::SonarThread(QObject* pParent) : QThread(pParent)
{
    mMinIntensity;
    mMaxIntensity;
    mFrameIndex = 0;
    mTotalFrames = 0;
    mFps = 30.0;
    mState = PlaybackState::Stop;
    mIsRunning = true;
    mIsPending = false;
}

// virtual
SonarThread::~SonarThread()
{
}

void
SonarThread::run()
{
    mIsRunning = true;
    unsigned long sleep_msec = 1000;
    while (mIsRunning && !isInterruptionRequested())
    {
        {
            QMutexLocker locker(&mMutex);
            if (mIsPending && !mPendingFilePath.isEmpty())
            {
                mCapture.release();
                mCapture.open(mPendingFilePath.toStdString());
                if (mCapture.isOpened())
                {
                    mFps = mCapture.get(cv::CAP_PROP_FPS);
                    mTotalFrames = static_cast<int>(mCapture.get(cv::CAP_PROP_FRAME_COUNT));
                    mFrameIndex = 0;
                    mFilePath = mPendingFilePath;
                    mIsPending = false;
                    mState = PlaybackState::Play;
                    emit fileChanged(mFilePath);
                }
            }
        }

        {
            QMutexLocker locker(&mMutex);
            if (mState == PlaybackState::Play || mState == PlaybackState::FastForward ||
                mState == PlaybackState::Rewind)
            {
                if ((mState == PlaybackState::Rewind && mFrameIndex <= 0) ||
                    ((mState == PlaybackState::Play || mState == PlaybackState::FastForward) &&
                     mFrameIndex >= (mTotalFrames - 1)))
                {
                    mState = PlaybackState::Stop;
                    emit playbackStopped(mFrameIndex);
                }

                if (mState != PlaybackState::Stop)
                {
                    int offset = 0;
                    if (mState == PlaybackState::Play)
                        offset = 1;
                    else if (mState == PlaybackState::FastForward)
                        offset = 30;
                    else if (mState == PlaybackState::Rewind)
                        offset = -30;

                    int next = mFrameIndex + offset;

                    if (next < 0)
                        next = 0;
                    if (next >= mTotalFrames)
                        next = mTotalFrames - 1;

                    mCapture.set(cv::CAP_PROP_POS_FRAMES, next);
                    mFrameIndex = next;

                    cv::Mat frame;
                    if (mCapture.read(frame))
                    {
                        if (frame.channels() == 1)
                            cv::cvtColor(frame, frame, cv::COLOR_GRAY2RGB);

                        frame.setTo(mMinIntensity, frame < mMinIntensity);
                        frame.setTo(mMaxIntensity, frame > mMaxIntensity);

                        QImage image(frame.data, frame.cols, frame.rows, frame.step,
                                     QImage::Format_RGB888);
                        emit frameReady(image.copy());
                        sleep_msec = static_cast<unsigned long>(1000.0 / mFps);
                    }
                }
            }
        }
        msleep(sleep_msec);
    }
    mCapture.release();
}

void
SonarThread::terminate()
{
    QMutexLocker locker(&mMutex);
    mIsRunning = false;
}

void
SonarThread::setParams(int minI, int maxI)
{
    QMutexLocker locker(&mMutex);
    mMinIntensity = minI;
    mMaxIntensity = maxI;
}

void
SonarThread::setFilePath(const QString& path)
{
    QMutexLocker locker(&mMutex);
    mPendingFilePath = path;
    mIsPending = true;
}

void
SonarThread::setFramePosition(int index)
{
    QMutexLocker locker(&mMutex);
    mCapture.set(cv::CAP_PROP_POS_FRAMES, index);
}

void
SonarThread::play()
{
    QMutexLocker locker(&mMutex);
    if (mState == PlaybackState::Stop)
    {
        mFrameIndex = 0;
        mCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
    }
    mState = PlaybackState::Play;
}

void
SonarThread::pause()
{
    QMutexLocker locker(&mMutex);
    mState = PlaybackState::Pause;
}

void
SonarThread::stop()
{
    QMutexLocker locker(&mMutex);
    mState = PlaybackState::Stop;
}

void
SonarThread::fastForward()
{
    QMutexLocker locker(&mMutex);
    mState = PlaybackState::FastForward;
}

void
SonarThread::rewind()
{
    QMutexLocker locker(&mMutex);
    mState = PlaybackState::Rewind;
}

int
SonarThread::totalFrameCount() const
{
    QMutexLocker locker(&mMutex);
    return mTotalFrames;
}

int
SonarThread::currentFrameIndex() const
{
    QMutexLocker locker(&mMutex);
    return mFrameIndex;
}

QString
SonarThread::currentFilePath() const
{
    QMutexLocker locker(&mMutex);
    return mFilePath;
}

std::chrono::milliseconds
SonarThread::elapsedDuration() const
{
    QMutexLocker locker(&mMutex);
    double seconds = mFrameIndex / mFps;
    return std::chrono::milliseconds(static_cast<int64_t>(seconds * 1000));
}

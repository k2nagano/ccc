#pragma once

#include "SonarThread.hh"
#include "SonarWidget.hh"
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QWidget>

class SonarPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit SonarPlayer(const QString& filePath, float range, int min_intensity,
                         int max_intensity);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

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
    SonarWidget* mpSonarWidget;
    SonarThread* mpSonarThread;

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
    QColor mForegroundColor;
    QColor mBackgroundColor;
    QColor mMinIntensityColor;
    QColor mMaxIntensityColor;
    QSlider* mpSliderFramePosition;
    QLineEdit* mpLineEditFilePath;
    // QString mFilePath;
};

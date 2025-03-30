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
    void togglePlayPause();
    void fastForward();
    void rewind();
    void updateFrame(const QImage& frame);
    void setFramePosition(int pos);
    void handleFileChanged(const QString& newPath);
    void handlePlaybackStopped(int frameIndex);

private:
    SonarWidget* widget;
    SonarThread* thread;

    QPushButton* playPauseBtn;
    QPushButton* fastForwardBtn;
    QPushButton* rewindBtn;
    QDoubleSpinBox* rangeSpinBox;
    QSpinBox* minISpin;
    QSpinBox* maxISpin;
    QPushButton* minColorBtn;
    QPushButton* maxColorBtn;
    QPushButton* fgColorBtn;
    QPushButton* bgColorBtn;
    QColor fgColor;
    QColor bgColor;
    QColor minColor;
    QColor maxColor;
    QSlider* frameSlider;
    QLineEdit* filePathEdit;
    // QString filePath;
};

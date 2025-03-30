#include "SonarPlayer.hh"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

SonarPlayer::SonarPlayer(const QString& filePath, float range, int minI, int maxI)
// : filePath(filePath)
{
    printf("%f %d %d\n", range, minI, maxI);
    setAcceptDrops(true);

    widget = new SonarWidget;
    QSettings s("uro_sonar", "sonar_player");
    widget->setRange(range);
    // if (s.contains("range"))
    //     widget->setRange(s.value("range").toDouble());
    if (s.contains("minIntensity"))
        widget->setMinIntensity(s.value("minIntensity").toInt());
    if (s.contains("maxIntensity"))
        widget->setMaxIntensity(s.value("maxIntensity").toInt());
    if (s.contains("minColor"))
        widget->setMinColor(s.value("minColor").value<QColor>());
    if (s.contains("maxColor"))
        widget->setMaxColor(s.value("maxColor").value<QColor>());
    if (s.contains("fgColor"))
        widget->setFgColor(s.value("fgColor").value<QColor>());
    if (s.contains("bgColor"))
        widget->setBgColor(s.value("bgColor").value<QColor>());

    thread = new SonarThread(this);
    thread->setParams(range, minI, maxI);
    thread->setFilePath(filePath);

    connect(thread, &SonarThread::frame_ready, this, &SonarPlayer::updateFrame);
    connect(thread, &SonarThread::file_changed, this, &SonarPlayer::handleFileChanged);
    connect(thread, &SonarThread::playbackStopped, this, &SonarPlayer::handlePlaybackStopped);

    playPauseBtn = new QPushButton("Pause");
    fastForwardBtn = new QPushButton("FastForward");
    rewindBtn = new QPushButton("Rewind");

    connect(playPauseBtn, &QPushButton::clicked, this, &SonarPlayer::togglePlayPause);
    connect(fastForwardBtn, &QPushButton::clicked, this, &SonarPlayer::fastForward);
    connect(rewindBtn, &QPushButton::clicked, this, &SonarPlayer::rewind);

    // range設定
    rangeSpinBox = new QDoubleSpinBox;
    rangeSpinBox->setRange(1.0, 100.0);
    rangeSpinBox->setValue(range);
    connect(rangeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [=](double value) {
                widget->setRange(value);
                thread->setParams(value, minI, maxI);
            });

    // MinIntensity / MaxIntensity
    QSpinBox* minISpin = new QSpinBox;
    QSpinBox* maxISpin = new QSpinBox;
    minISpin->setRange(0, 255);
    maxISpin->setRange(0, 255);
    minISpin->setValue(minI);
    maxISpin->setValue(maxI);
    connect(minISpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [=](int value) { thread->setParams(rangeSpinBox->value(), value, maxISpin->value()); });
    connect(maxISpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [=](int value) { thread->setParams(rangeSpinBox->value(), minISpin->value(), value); });

    fgColorBtn = new QPushButton("FgColor");
    bgColorBtn = new QPushButton("BgColor");
    minColorBtn = new QPushButton("MinColor");
    maxColorBtn = new QPushButton("MaxColor");

    connect(fgColorBtn, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(fgColor, this);
        if (color.isValid())
        {
            fgColor = color;
            widget->setFgColor(color);
        }
    });

    connect(bgColorBtn, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(bgColor, this);
        if (color.isValid())
        {
            bgColor = color;
            widget->setBgColor(color);
        }
    });

    connect(minColorBtn, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(minColor, this);
        if (color.isValid())
        {
            minColor = color;
            widget->setMinColor(color);
        }
    });

    connect(maxColorBtn, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(maxColor, this);
        if (color.isValid())
        {
            maxColor = color;
            widget->setMaxColor(color);
        }
    });

    QHBoxLayout* spinLayout = new QHBoxLayout;
    spinLayout->addWidget(rangeSpinBox);
    spinLayout->addWidget(minISpin);
    spinLayout->addWidget(maxISpin);
    QHBoxLayout* colorLayout = new QHBoxLayout;
    colorLayout->addWidget(fgColorBtn);
    colorLayout->addWidget(bgColorBtn);
    colorLayout->addWidget(minColorBtn);
    colorLayout->addWidget(maxColorBtn);

    frameSlider = new QSlider(Qt::Horizontal);
    frameSlider->setRange(0, thread->totalFrameCount());
    connect(frameSlider, &QSlider::sliderMoved, this, &SonarPlayer::setFramePosition);

    filePathEdit = new QLineEdit(filePath);
    filePathEdit = new QLineEdit(filePath);
    filePathEdit->setReadOnly(true);
    filePathEdit->setFocusPolicy(Qt::ClickFocus);
    filePathEdit->setStyleSheet("background: palette(window);");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(widget);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(playPauseBtn);
    buttonLayout->addWidget(fastForwardBtn);
    buttonLayout->addWidget(rewindBtn);
    layout->addLayout(buttonLayout);
    layout->addLayout(spinLayout);
    // layout->addLayout(colorLayout);
    layout->addWidget(frameSlider);
    layout->addWidget(filePathEdit);

    thread->start(); // 初期ファイルを自動再生
}

void
SonarPlayer::togglePlayPause()
{
    if (playPauseBtn->text() == "Pause")
    {
        thread->pause();
        playPauseBtn->setText("Play");
    }
    else
    {
        thread->play();
        playPauseBtn->setText("Pause");
    }
}

void
SonarPlayer::fastForward()
{
    thread->fastForward();
}

void
SonarPlayer::rewind()
{
    thread->rewind();
}

void
SonarPlayer::updateFrame(const QImage& frame)
{
    widget->setFrame(frame);
    int current = thread->currentFrameIndex();
    frameSlider->setValue(current);
}

void
SonarPlayer::setFramePosition(int pos)
{
    thread->setFramePosition(pos);
}

void
SonarPlayer::handleFileChanged(const QString& newPath)
{
    filePathEdit->setText(newPath);
    frameSlider->setValue(0);
    frameSlider->setMaximum(thread->totalFrameCount());
    playPauseBtn->setText("Pause"); // 再生中
}

void
SonarPlayer::handlePlaybackStopped(int frameIndex)
{
    frameSlider->setValue(frameIndex);
    playPauseBtn->setText("Play");
}

void
SonarPlayer::closeEvent(QCloseEvent* event)
{
    QSettings s("uro_sonar", "sonar_player");
    s.setValue("range", widget->range());
    s.setValue("minIntensity", widget->minIntensity());
    s.setValue("maxIntensity", widget->maxIntensity());
    s.setValue("minColor", widget->minColor());
    s.setValue("maxColor", widget->maxColor());
    s.setValue("fgColor", widget->fgColor());
    s.setValue("bgColor", widget->bgColor());
    QWidget::closeEvent(event);
}
void
SonarPlayer::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void
SonarPlayer::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty())
    {
        QString newFile = urls.first().toLocalFile();
        if (!newFile.isEmpty())
        {
            thread->setFilePath(newFile);
        }
    }
}

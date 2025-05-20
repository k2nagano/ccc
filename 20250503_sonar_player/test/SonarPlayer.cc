#include "SonarPlayer.hh"

// explicit
SonarPlayer::SonarPlayer(const QString& mkvPath, double swath, double range, int minIntensity,
                         int maxIntensity, QWidget* pParent)
    : QWidget(pParent), mSwath(swath), mRange(range), mMinIntensity(minIntensity),
      mMaxIntensity(maxIntensity), mForegroundColor(Qt::white), mBackgroundColor(Qt::black),
      mMinIntensityColor(Qt::yellow), mMaxIntensityColor(Qt::red)

{
    setAcceptDrops(true);
    // キー操作を受け取れるように
    setFocusPolicy(Qt::StrongFocus);

    // コンストラクタの初めで、まずウィジェット生成
    mpSonarWidget = new SonarWidget;

    mpPushButtonPlay = new QPushButton;
    mpPushButtonPause = new QPushButton;
    mpPushButtonStop = new QPushButton;
    mpPushButtonFastForward = new QPushButton;
    mpPushButtonRewind = new QPushButton;

    auto* st = this->style();

    mpPushButtonPlay->setIcon(st->standardIcon(QStyle::SP_MediaPlay));
    mpPushButtonPause->setIcon(st->standardIcon(QStyle::SP_MediaPause));
    mpPushButtonStop->setIcon(st->standardIcon(QStyle::SP_MediaStop));
    mpPushButtonFastForward->setIcon(st->standardIcon(QStyle::SP_MediaSkipForward));
    mpPushButtonRewind->setIcon(st->standardIcon(QStyle::SP_MediaSkipBackward));

    // アイコンサイズを指定
    QSize iconSize(20, 20);
    mpPushButtonPlay->setIconSize(iconSize);
    mpPushButtonPause->setIconSize(iconSize);
    mpPushButtonStop->setIconSize(iconSize);
    mpPushButtonFastForward->setIconSize(iconSize);
    mpPushButtonRewind->setIconSize(iconSize);

    connect(mpPushButtonPlay, &QPushButton::clicked, this, &SonarPlayer::play);
    connect(mpPushButtonPause, &QPushButton::clicked, this, &SonarPlayer::pause);
    connect(mpPushButtonStop, &QPushButton::clicked, this, &SonarPlayer::stop);
    connect(mpPushButtonFastForward, &QPushButton::clicked, this, &SonarPlayer::fastForward);
    connect(mpPushButtonRewind, &QPushButton::clicked, this, &SonarPlayer::rewind);

    // スワス／レンジ／強度用 SpinBox／Labels
    auto* lblSwath = new QLabel("Swath[deg]", this);
    auto* lblRange = new QLabel("Range[m]", this);
    auto* lblMinInt = new QLabel("MinIntensity", this);
    auto* lblMaxInt = new QLabel("MaxIntensity", this);

    lblSwath->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblRange->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblMinInt->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblMaxInt->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    mpDoubleSpinBoxSwath = new QDoubleSpinBox;
    mpDoubleSpinBoxRange = new QDoubleSpinBox;
    mpSpinBoxMinIntensity = new QSpinBox;
    mpSpinBoxMaxIntensity = new QSpinBox;

    mpDoubleSpinBoxSwath->setValue(swath);
    mpDoubleSpinBoxRange->setValue(range);
    mpDoubleSpinBoxSwath->setRange(1.0, 180.0);
    mpDoubleSpinBoxRange->setRange(1.0, 100.0);
    mpSpinBoxMinIntensity->setRange(0, 255);
    mpSpinBoxMaxIntensity->setRange(0, 255);

    connect(mpDoubleSpinBoxRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [=](double value) {
                mpSonarWidget->setRange(value);
                mpSonarThread->setParams(minIntensity, maxIntensity);
            });

    mpSpinBoxMinIntensity->setValue(minIntensity);
    mpSpinBoxMaxIntensity->setValue(maxIntensity);
    connect(mpSpinBoxMinIntensity, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [=](int value) { mpSonarThread->setParams(value, mpSpinBoxMaxIntensity->value()); });
    connect(mpSpinBoxMaxIntensity, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [=](int value) { mpSonarThread->setParams(mpSpinBoxMinIntensity->value(), value); });

    // カラー選択ボタン
    mpPushButtonMinIntensityColor = new QPushButton("MinIntensityColor");
    mpPushButtonMaxIntensityColor = new QPushButton("MaxIntensityColor");
    mpPushButtonForegroundColor = new QPushButton("ForegroundColor");
    mpPushButtonBackgroundColor = new QPushButton("BackgroundColor");

    connect(mpPushButtonForegroundColor, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(mForegroundColor, this);
        if (color.isValid())
        {
            mForegroundColor = color;
            mpSonarWidget->setForegroundColor(color);
        }
    });

    connect(mpPushButtonBackgroundColor, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(mBackgroundColor, this);
        if (color.isValid())
        {
            mBackgroundColor = color;
            mpSonarWidget->setBackgroundColor(color);
        }
    });

    connect(mpPushButtonMinIntensityColor, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(mMinIntensityColor, this);
        if (color.isValid())
        {
            mMinIntensityColor = color;
            mpSonarWidget->setMinIntensityColor(color);
        }
    });

    connect(mpPushButtonMaxIntensityColor, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(mMaxIntensityColor, this);
        if (color.isValid())
        {
            mMaxIntensityColor = color;
            mpSonarWidget->setMaxIntensityColor(color);
        }
    });

    // --- レイアウト構築 ---
    auto* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(mpSonarWidget);

    // １段目：再生コントロール＋パラメータ
    auto* h1 = new QHBoxLayout;
    h1->addWidget(mpPushButtonPlay);
    h1->addWidget(mpPushButtonPause);
    h1->addWidget(mpPushButtonStop);
    h1->addWidget(mpPushButtonFastForward);
    h1->addWidget(mpPushButtonRewind);
    h1->addSpacing(20);
    h1->addWidget(lblSwath);
    h1->addWidget(mpDoubleSpinBoxSwath);
    h1->addWidget(lblRange);
    h1->addWidget(mpDoubleSpinBoxRange);
    h1->addWidget(lblMinInt);
    h1->addWidget(mpSpinBoxMinIntensity);
    h1->addWidget(lblMaxInt);
    h1->addWidget(mpSpinBoxMaxIntensity);
    // h1->addStretch();
    mainLayout->addLayout(h1);

    // ２段目：カラー選択
    auto* h2 = new QHBoxLayout;
    h2->addWidget(mpPushButtonMinIntensityColor);
    h2->addWidget(mpPushButtonMaxIntensityColor);
    h2->addWidget(mpPushButtonForegroundColor);
    h2->addWidget(mpPushButtonBackgroundColor);
    // h2->addStretch();
    mainLayout->addLayout(h2);

    mpSliderFramePosition = new QSlider(Qt::Horizontal);
    connect(mpSliderFramePosition, &QSlider::sliderMoved, this, &SonarPlayer::setFramePosition);

    mpLineEditMkvPath = new QLineEdit(mkvPath);
    mpLineEditMkvPath->setReadOnly(true);
    mpLineEditMkvPath->setFocusPolicy(Qt::ClickFocus);
    mpLineEditMkvPath->setStyleSheet("background: palette(window);");

    mainLayout->addWidget(mpSliderFramePosition);
    mainLayout->addWidget(mpLineEditMkvPath);

    setLayout(mainLayout);

    mpSonarThread = new SonarThread;
    mpSonarThread->setParams(minIntensity, maxIntensity);
    mpSonarThread->setFilePath(mkvPath);
    mpSliderFramePosition->setRange(0, mpSonarThread->totalFrameCount());

    connect(mpSonarThread, &SonarThread::frameReady, this, &SonarPlayer::updateFrame);
    connect(mpSonarThread, &SonarThread::fileChanged, this, &SonarPlayer::handleFileChanged);
    connect(mpSonarThread, &SonarThread::playbackStopped, this,
            &SonarPlayer::handlePlaybackStopped);

    mpSonarThread->start(); // 初期ファイルを自動再生
}

// virtual
SonarPlayer::~SonarPlayer()
{
}

void
SonarPlayer::play()
{
    mpSonarThread->play();
}
void
SonarPlayer::pause()
{
    mpSonarThread->pause();
}
void
SonarPlayer::stop()
{
    mpSonarThread->stop();
}
void
SonarPlayer::fastForward()
{
    mpSonarThread->fastForward();
}
void
SonarPlayer::rewind()
{
    mpSonarThread->rewind();
}

void
SonarPlayer::updateFrame(const QImage& frame)
{
    mpSonarWidget->setFrame(frame);
    int current = mpSonarThread->currentFrameIndex();
    mpSliderFramePosition->setValue(current);
}

void
SonarPlayer::setFramePosition(int pos)
{
    mpSonarThread->setFramePosition(pos);
}

void
SonarPlayer::handleFileChanged(const QString& newPath)
{
    mpLineEditMkvPath->setText(newPath);
    mpSliderFramePosition->setValue(0);
    mpSliderFramePosition->setMaximum(mpSonarThread->totalFrameCount());
}

void
SonarPlayer::handlePlaybackStopped(int frameIndex)
{
    mpSliderFramePosition->setValue(frameIndex);
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
            mpSonarThread->setFilePath(newFile);
        }
    }
}

void
SonarPlayer::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key())
    {
    case Qt::Key_Space:
        if (mMaintainedMode == Mode::Play)
        {
            mMaintainedMode = Mode::Pause;
            mpSonarThread->pause();
        }
        else
        {
            mMaintainedMode = Mode::Play;
            mpSonarThread->play();
        }
        break;
    case Qt::Key_Right:
        mpSonarThread->fastForward();
        break;
    case Qt::Key_Left:
        mpSonarThread->rewind();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void
SonarPlayer::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
    {
        // 離せば元の再生／一時停止状態に戻す
        if (mMaintainedMode == Mode::Play)
            mpSonarThread->play();
        else
            mpSonarThread->pause();
    }
    else
    {
        QWidget::keyReleaseEvent(event);
    }
}

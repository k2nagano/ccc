#include "SonarPlayer.hh"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

SonarPlayer::SonarPlayer(const QString& filePath, float range, int minI, int maxI)
// : mFilePath(filePath)
{
    printf("%f %d %d\n", range, minI, maxI);
    setAcceptDrops(true);

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

    mpDoubleSpinBoxSwath->setValue(120.0);
    mpDoubleSpinBoxRange->setValue(range);
    mpDoubleSpinBoxSwath->setRange(1.0, 180.0);
    mpDoubleSpinBoxRange->setRange(1.0, 100.0);
    mpSpinBoxMinIntensity->setRange(0, 255);
    mpSpinBoxMaxIntensity->setRange(0, 255);

    connect(mpDoubleSpinBoxRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [=](double value) {
                mpSonarWidget->setRange(value);
                mpSonarThread->setParams(minI, maxI);
            });

    mpSpinBoxMinIntensity->setValue(minI);
    mpSpinBoxMaxIntensity->setValue(maxI);
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

    mpLineEditFilePath = new QLineEdit(filePath);
    mpLineEditFilePath->setReadOnly(true);
    mpLineEditFilePath->setFocusPolicy(Qt::ClickFocus);
    mpLineEditFilePath->setStyleSheet("background: palette(window);");

    mainLayout->addWidget(mpSliderFramePosition);
    mainLayout->addWidget(mpLineEditFilePath);

    setLayout(mainLayout);

    mpSonarThread = new SonarThread;
    mpSonarThread->setParams(minI, maxI);
    mpSonarThread->setFilePath(filePath);
    mpSliderFramePosition->setRange(0, mpSonarThread->totalFrameCount());

    connect(mpSonarThread, &SonarThread::frameReady, this, &SonarPlayer::updateFrame);
    connect(mpSonarThread, &SonarThread::fileChanged, this, &SonarPlayer::handleFileChanged);
    connect(mpSonarThread, &SonarThread::playbackStopped, this,
            &SonarPlayer::handlePlaybackStopped);

    QSettings s("uro_sonar", "sonar_player");
    if (s.contains("Swath"))
    {
        double sw = s.value("Swath").toDouble();
        mpSonarWidget->setSwath(sw);
        mpDoubleSpinBoxSwath->setValue(sw);
    }
    if (s.contains("Range"))
    {
        double rg = s.value("Range").toDouble();
        mpSonarWidget->setRange(rg);
        mpDoubleSpinBoxRange->setValue(rg);
    }
    if (s.contains("MinIntensity"))
    {
        int mi = s.value("MinIntensity").toInt();
        mpSonarWidget->setMinIntensity(mi);
        mpSpinBoxMinIntensity->setValue(mi);
    }
    if (s.contains("MaxIntensity"))
    {
        int ma = s.value("MaxIntensity").toInt();
        mpSonarWidget->setMaxIntensity(ma);
        mpSpinBoxMaxIntensity->setValue(ma);
    }
    if (s.contains("MinIntensityColor"))
    {
        QColor c = s.value("MinIntensityColor").value<QColor>();
        mMinIntensityColor = c;
        mpSonarWidget->setMinIntensityColor(c);
        // ボタンの表示色も更新する
        mpPushButtonMinIntensityColor->setStyleSheet(QString("background:%1").arg(c.name()));
    }
    if (s.contains("MaxIntensityColor"))
    {
        QColor c = s.value("MaxIntensityColor").value<QColor>();
        mMinIntensityColor = c;
        mpSonarWidget->setMaxIntensityColor(c);
        // ボタンの表示色も更新する
        mpPushButtonMaxIntensityColor->setStyleSheet(QString("background:%1").arg(c.name()));
    }
    if (s.contains("ForegroundColor"))
    {
        QColor c = s.value("ForegroundColor").value<QColor>();
        mForegroundColor = c;
        mpSonarWidget->setForegroundColor(c);
        // ボタンの表示色も更新する
        mpPushButtonForegroundColor->setStyleSheet(QString("background:%1").arg(c.name()));
    }
    if (s.contains("BackgroundColor"))
    {
        QColor c = s.value("BackgroundColor").value<QColor>();
        mBackgroundColor = c;
        mpSonarWidget->setBackgroundColor(c);
        // ボタンの表示色も更新する
        mpPushButtonBackgroundColor->setStyleSheet(QString("background:%1").arg(c.name()));
    }

    mpSonarThread->start(); // 初期ファイルを自動再生
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
    mpLineEditFilePath->setText(newPath);
    mpSliderFramePosition->setValue(0);
    mpSliderFramePosition->setMaximum(mpSonarThread->totalFrameCount());
}

void
SonarPlayer::handlePlaybackStopped(int frameIndex)
{
    mpSliderFramePosition->setValue(frameIndex);
}

void
SonarPlayer::closeEvent(QCloseEvent* event)
{
    QSettings s("uro_sonar", "sonar_player");
    s.setValue("Swath", mpSonarWidget->swath());
    s.setValue("Range", mpSonarWidget->range());
    s.setValue("MinIntensity", mpSonarWidget->minIntensity());
    s.setValue("MaxIntensity", mpSonarWidget->maxIntensity());
    s.setValue("MinIntensityColor", mpSonarWidget->minIntensityColor());
    s.setValue("MaxIntensityColor", mpSonarWidget->maxIntensityColor());
    s.setValue("ForegroundColor", mpSonarWidget->foregroundColor());
    s.setValue("BackgroundColor", mpSonarWidget->backgroundColor());
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
            mpSonarThread->setFilePath(newFile);
        }
    }
}

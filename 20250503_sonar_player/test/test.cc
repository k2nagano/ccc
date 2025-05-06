#include "SonarPlayer.hh"
#include <QCommandLineParser>
#include <QFileDialog>
#include <QtWidgets>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // --- コマンドラインパーサ設定 ---
    QCoreApplication::setApplicationName("SonarPlayer");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Play and visualize sonar data from an MKV file");
    parser.addHelpOption();
    parser.addVersionOption();

    // MKV ファイルパス
    QCommandLineOption mkvOpt(QStringList{"m", "mkv"}, "Sonar data MKV file path", "MKV");
    parser.addOption(mkvOpt);

    // 扇形開口角度
    QCommandLineOption swathOpt(QStringList{"s", "swath"}, "Fan opening angle [deg]", "SWATH");
    parser.addOption(swathOpt);

    // 探索レンジ
    QCommandLineOption rangeOpt(QStringList{"r", "range"}, "Sonar exploration range (m)", "RANGE");
    parser.addOption(rangeOpt);

    // 最小強度
    QCommandLineOption minIntOpt(QStringList{"i", "min-intensity"}, "Minimum intensity for display",
                                 "MIN_INTENSITY");
    parser.addOption(minIntOpt);

    // 最大強度
    QCommandLineOption maxIntOpt(QStringList{"x", "max-intensity"}, "Maximum intensity for display",
                                 "MAX_INTENSITY");
    parser.addOption(maxIntOpt);

    parser.process(app);

    QString mkvPath;
    if (parser.isSet(mkvOpt))
    {
        mkvPath = parser.value(mkvOpt);
    }
    else
    {
        mkvPath =
            QFileDialog::getOpenFileName(nullptr, "MKVファイルを選択", "", "動画ファイル (*.mkv)");
    }
    if (mkvPath.isEmpty())
    {
        return -1;
    }

    // 値の取得（未設定の場合は各自デフォルトを設定）
    double swath = parser.isSet(swathOpt) ? parser.value(swathOpt).toDouble() : 120.0;
    double range = parser.isSet(rangeOpt) ? parser.value(rangeOpt).toDouble() : 30.0;
    int minIntensity = parser.isSet(minIntOpt) ? parser.value(minIntOpt).toInt() : 0;
    int maxIntensity = parser.isSet(maxIntOpt) ? parser.value(maxIntOpt).toInt() : 255;

    SonarPlayer w(mkvPath, swath, range, minIntensity, maxIntensity);
    w.show();
    app.exec();
    return 0;
}

#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>
#include "SonarPlayer.hh"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Sonar MKV Player");

    QCommandLineParser parser;
    parser.setApplicationDescription("Sonar MKV Player");
    parser.addHelpOption();
    parser.addOption({"range", "最大レンジ（メートル）", "range", "10"});
    parser.addOption({"min_intensity", "最小強度", "min", "0"});
    parser.addOption({"max_intensity", "最大強度", "max", "255"});
    parser.addPositionalArgument("file", "読み込むMKVファイルのパス");

    parser.process(app);

    QString filePath;
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        filePath = args[0];
    } else {
        filePath = QFileDialog::getOpenFileName(nullptr, "MKVファイルを選択", "", "動画ファイル (*.mkv)");
        if (filePath.isEmpty()) return 0;
    }

    float range = parser.value("range").toFloat();
    int min_intensity = parser.value("min_intensity").toInt();
    int max_intensity = parser.value("max_intensity").toInt();

    SonarPlayer player(filePath, range, min_intensity, max_intensity);
    // player.resize(800, 800);
    player.show();

    return app.exec();
}

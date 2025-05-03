#include "Widget.hh"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.setWindowTitle("SonarPanel");
    w.show();
    return a.exec();
}

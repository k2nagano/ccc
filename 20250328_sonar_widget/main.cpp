#include <QApplication>
#include "SonarWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SonarWidget w;
    w.show();
    return app.exec();
}


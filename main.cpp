#include "mainwindow.h"

#include <QApplication>
#include <QFile>

void setStyle(QApplication& app, const QString& file) {
    QFile styleSheetFile(file);
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(styleSheetFile.readAll());
    app.setStyleSheet(styleSheet);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    //setStyle(app, ":/res/Combinear.qss");

    MainWindow w;
    w.show();
    return app.exec();
}

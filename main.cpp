#include <QApplication>
#include <QTextCodec>
#include <QDebug>
#include "MainWindow.h"
#include "stock/stockdata.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QTextCodec::setCodecForLocale(codec) ;
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

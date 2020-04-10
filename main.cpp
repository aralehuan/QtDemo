#include <QApplication>
#include <QTextCodec>
#include "MainWindow.h"
#include "stockdata.h"
int main(int argc, char *argv[])
{
    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QTextCodec::setCodecForLocale(codec) ;
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

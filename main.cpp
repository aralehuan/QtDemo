#include "MainWindow.h"
#include <QApplication>
#include "stockdata.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    //StockData sd;
    //sd.ReqData(0);
    return a.exec();
}

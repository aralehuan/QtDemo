#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QSystemTrayIcon>
#include <QMenu>
#include "stock/StockMgr.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_k_select(KData* k);
    void onTaskFinished(QSharedPointer<StockTask> sharetask);
    void onSyncProgress(float progress);
    void onMessage(int type, QString msg);
    void onActivatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuAutoStartTriggered(bool sel);
    void onTrayMenuCloseTriggered();

    void onTableSort(int col);
    void on_stockTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_stockTable_customContextMenuRequested(const QPoint &pos);
    void on_toolSync_triggered();
    void on_toolAnalyse_triggered();
    void on_toolSave_triggered();
    void on_toolBlacklist_triggered();
    void on_toolSafeSync_triggered(bool checked);
    void on_tookCheck_triggered();
    void on_menuSync_triggered();
    void on_menuSave_triggered();
    void on_menuAnalyse_triggered();
    void on_menuBlacklist_triggered(bool checked);
    void on_menuReset_triggered();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *mSysTrayIcon;
    Qt::SortOrder* sorts;
    QMenu* tableMenu;
    Stock* selectedStock;
    int  timerID;
    int  syncStockIndex;


    void init();
    bool event(QEvent *event);
    void closeEvent(QCloseEvent * event);
    void timerEvent(QTimerEvent *event);
    //设置开机启动
    void setAutoStart(bool autoRun);
    bool getAutoStart();
    QString toMoney(double money);
};

#endif // MAINWINDOW_H

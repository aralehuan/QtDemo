#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QLabel>
#include <QProgressBar>
#include "stock/StockMgr.h"
#include "MyToolAction.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum TB
    {
        DM, //代码
        MC, //名称
        HY, //行业
        DQ,//地区
        SH,//上市
        HMD,//黑名单
        ZT,//状态
        FXSJ,//分析时间
        KZ,//看涨
        JG,//看涨结果(第2交易日涨幅)
        XJ,//现价
        ZF,//涨幅
        NZDB,//年涨比
        YZDB,//月涨跌比
        RZDB,//7日涨跌比
        LZ,//连涨
        LB,//量比
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_k_select(KData* k);
    void onTaskStart(StockTask* task);
    void onTaskFinished(QSharedPointer<StockTask> sharetask);
    void onSyncProgress(float progress);
    void onMessage(int type, QString msg);
    void onActivatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuAutoStartTriggered(bool sel);
    void onTrayMenuCloseTriggered();
    void onTrayMenuConsoleTriggered();
    void onCheckReset(){checkIndex=0;}
    void onAnalyseReset(){analyseIndex=0;}
    void onSafeSyncReset(){syncIndex=0;}

    void onTableSort(int col);
    void on_stockTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_stockTable_customContextMenuRequested(const QPoint &pos);
    void on_toolSave_triggered();
    void on_toolBlacklist_triggered();
    void on_toolQuickSync_triggered();
    void on_toolAnalyse_triggered(bool checked);
    void on_toolSafeSync_triggered(bool checked);
    void on_tookCheck_triggered(bool checked);
    void on_toolRemoveBehind_triggered();
    void on_toolRemoveBefore_triggered();
    void on_toolExport_triggered();
    void on_menuSync_triggered();
    void on_menuCheck_triggered();
    void on_menuAnalyse_triggered();
    void on_menuBlacklist_triggered(bool checked);
    void on_menuReset_triggered();  
    void on_menuLookUp_triggered();
    void on_menuLookDown_triggered();
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *mSysTrayIcon;
    Qt::SortOrder* sorts;
    QMenu* tableMenu;
    Stock* selectedStock;
    int selectedDate;
    int saveTimer;
    int  syncTimer;
    int  syncIndex;
    int  checkTimer;
    int  checkIndex;
    int  analyseTimer;
    int  analyseIndex;
    QLabel* minDateLB;
    QLabel* maxDateLB;
    QLabel* taskCountLB;
    QLabel* savingLB;
    QProgressBar* progressBar;
    MyToolAction* actionCheck;
    MyToolAction* actionSafeSync;
    MyToolAction* actionAnalyse;
    QLineEdit* dateEdit;

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

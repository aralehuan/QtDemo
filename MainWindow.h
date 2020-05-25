#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
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
    void on_toolSync_triggered();
    void on_toolAnalyse_triggered();

    void on_k_select(KData* k);
    void on_stockTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void onTaskFinished(StockTask* task);
    void onSyncProgress(float progress);
    void onMessage(int type, QString msg);

private:
    Ui::MainWindow *ui;
    void init();
    bool event(QEvent *event);
};

#endif // MAINWINDOW_H

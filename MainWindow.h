#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "AraleStock.h"

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
    void on_pushButton_clicked();

    void on_radioButton_toggled(bool checked);

    void on_actionAdd_triggered();

    void on_newWinButton_clicked();

    void on_listWidget_currentRowChanged(int currentRow);

    void on_req_stock(QString code);

    void on_k_select(KData* k);
private:
    Ui::MainWindow *ui;
    AraleStock mStock;
    void init();
};

#endif // MAINWINDOW_H

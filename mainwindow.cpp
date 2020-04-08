#include <QtDebug>
#include <QDate>
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "listitem.h"
#include "subwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->kmap, SIGNAL(selectK(KData*)), this, SLOT(on_k_select(KData*)) );

    QString err = mStock.init();
    if(err!=nullptr)
    {
        QMessageBox::critical(nullptr, "错误", err, QMessageBox::Cancel);
    }

    mStock.loadStocks();
    const QList<Stock*>& ls = mStock.getStocks();
    for(int i=0;i<100;/*ls.count();*/++i)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0,50));
        ui->listWidget->addItem(item);
        ListItem* li = new ListItem();
        li->init(i+1, ls[i]);
        li->li = item;
        ui->listWidget->setItemWidget(item,li);
        connect(li, SIGNAL(reqStock(QString)), this, SLOT(on_req_stock(QString)) );
    }
}

MainWindow::~MainWindow()
{
    mStock.deinit();
    delete ui;
}

void MainWindow::on_req_stock(QString code)
{
    mStock.reqStock(code);
}

void MainWindow::on_k_select(KData* k)
{
    if(k!=nullptr)
    {
        QDate date = QDate::fromString(k->date,"yyyy-MM-dd");
        QString ds = date.toString("yy/MM/dd/dddd").remove(9,2);
        ui->lbDate->setText(ds);
        ui->lbOpen->setText(QString::asprintf("开 %0.2f",k->open));
        ui->lbClose->setText(QString::asprintf("收 %0.2f",k->close));
        ui->lbHigh->setText(QString::asprintf("高 %0.2f",k->high));
        ui->lbLow->setText(QString::asprintf("低 %0.2f",k->low));
        ui->lbVolume->setText(QString::asprintf("量 %0.2f",k->volume));
        ui->lbAmount->setText(QString::asprintf("额 %0.2f",0));
        ui->lbChg->setText(QString::asprintf("涨 %0.2f",0));
    }
    else
    {
        ui->lbDate->setText("");
        ui->lbOpen->setText("");
        ui->lbClose->setText("");
        ui->lbHigh->setText("");
        ui->lbLow->setText("");
        ui->lbVolume->setText("");
        ui->lbAmount->setText("");
        ui->lbChg->setText("");
    }
}

void MainWindow::on_pushButton_clicked()
{
    ui->statusBar->showMessage("你点击了pushButton");
    QListWidgetItem* item = new QListWidgetItem();
    item->setSizeHint(QSize(0,50));
    ui->listWidget->addItem(item);
    ListItem* li = new ListItem();
    li->li = item;
    ui->listWidget->setItemWidget(item,li);
}

void MainWindow::on_radioButton_toggled(bool checked)
{
    QString s = checked?"true":"false";
    ui->statusBar->showMessage("你点击了radioButton:"+s);
}

void MainWindow::on_actionAdd_triggered()
{
    ui->statusBar->showMessage("你点击了工具栏actionAdd");
}

void MainWindow::on_newWinButton_clicked()
{
   // SubWindow* sw = new SubWindow();
   // sw->setWindowFlags(Qt::SubWindow|Qt::FramelessWindowHint);//无边框子窗口
    //sw->setParent(this);//嵌入式子窗口
    //sw->show();

    mStock.reqStock("688018");
}

void MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
    const QList<Stock*>& ls = mStock.getStocks();
    Stock* s = ls[currentRow];
    ui->kmap->setStock(s);
    ui->statusBar->showMessage(s->name);
}

void MainWindow::init()
{
}

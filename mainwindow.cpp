#pragma execution_character_set("utf-8")
#include <QtDebug>
#include <QDate>
#include <QScrollArea>
#include "MainWindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //设置风格
    //QString mainStyle("border:1px solid #FF0000;background:rgba(0, 0, 0,100%);color:white;");
    //setStyleSheet("border:1px solid #FF0000;background:rgba(0, 0, 0,100%);color:white;");
    //信号槽
    connect(ui->kmap, SIGNAL(selectK(KData*)), this, SLOT(on_k_select(KData*)) );
    connect(StockMgr::single(), SIGNAL(sendStockChanged(Stock*)), this, SLOT(onStockChanged(Stock*)), Qt::ConnectionType::QueuedConnection);
    //数据初始化
    StockMgr* sm = StockMgr::single();
    QString err = sm->init();
    if(err!=nullptr)
    {
        QMessageBox::critical(nullptr, "错误", err, QMessageBox::Cancel);
    }
    //显示股票列表
    const QList<Stock*>& ls = sm->getStocks();
    ui->stockTable-> setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
    ui->stockTable->setSelectionMode(QAbstractItemView::SingleSelection);//单选
    ui->stockTable->setSelectionBehavior(QAbstractItemView::SelectRows);//单击格也选中一行
    ui->stockTable->setAlternatingRowColors(true);
    ui->stockTable->setRowCount(ls.count());
    for(int i=0;i<ls.count();++i)
    {
        Stock*  sk = ls[i];
        ui->stockTable->setItem(i,0,new QTableWidgetItem(sk->code));
        ui->stockTable->setItem(i,1,new QTableWidgetItem(sk->name));
        ui->stockTable->setItem(i,2,new QTableWidgetItem(sk->industry));
        ui->stockTable->setItem(i,3,new QTableWidgetItem(sk->area));
        ui->stockTable->setItem(i,4,new QTableWidgetItem(QString::number(sk->marketTime, 10)));
    }
}

MainWindow::~MainWindow()
{
    StockMgr::single()->deinit();
    delete ui;
}

void MainWindow::on_req_stock(QString code)
{
    qDebug()<<code;
   //StockMgr::single()->reqStock(code);
}

void MainWindow::on_k_select(KData* k)
{
    if(k!=nullptr)
    {
        QDate date = QDate::fromString(k->strDate(),"yyyy-MM-dd");
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
    /*ui->statusBar->showMessage("你点击了pushButton");
    QListWidgetItem* item = new QListWidgetItem();
    item->setSizeHint(QSize(0,50));
    ui->listWidget->addItem(item);
    ListItem* li = new ListItem();
    li->li = item;
    ui->listWidget->setItemWidget(item,li);*/
}

void MainWindow::on_radioButton_toggled(bool checked)
{
   // QString s = checked?"true":"false";
    //ui->statusBar->showMessage("你点击了radioButton:"+s);
}

void MainWindow::on_actionAdd_triggered()
{
   // SubWindow* sw = new SubWindow();
    //sw->setWindowFlags(Qt::SubWindow|Qt::FramelessWindowHint);//无边框子窗口
    //sw->setParent(this);//嵌入式子窗口
    //sw->show();
   // ui->statusBar->showMessage("你点击了工具栏actionAdd");
}

void MainWindow::on_stockTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    const QList<Stock*>& ls = StockMgr::single()->getStocks();
    Stock* s = ls[currentRow];
    ui->kmap->setStock(s);
    ui->statusBar->showMessage(s->name);
}

void MainWindow::onStockChanged(Stock* s)
{
    ui->kmap->update();
}

void MainWindow::init()
{
}

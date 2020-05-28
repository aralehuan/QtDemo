#pragma execution_character_set("utf-8")
#include <QtDebug>
#include <QDate>
#include <QScrollArea>
#include <QSettings>
#include <QMenu>
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "LoadingDlg.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),selectedStock(nullptr),timerID(0),syncStockIndex(0)
{
    ui->setupUi(this);
    //设置系统托盘
    mSysTrayIcon = new QSystemTrayIcon(this);
    mSysTrayIcon->setIcon(QIcon(":/icon/Res/stock.jpg"));
    mSysTrayIcon->setToolTip(QString("选股软件"));
    connect(mSysTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(onActivatedSysTrayIcon(QSystemTrayIcon::ActivationReason)));
    mSysTrayIcon->show();
    QMenu* trayMenu = new QMenu();
    QAction* a = new QAction("开机启动", trayMenu);
    a->setCheckable(true);
    a->setChecked(this->getAutoStart());
    QAction* b = new QAction("退出程序", trayMenu);
    trayMenu->addAction(a);
    trayMenu->addAction(b);
    mSysTrayIcon->setContextMenu(trayMenu);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(onTrayMenuAutoStartTriggered(bool)));
    connect(b, SIGNAL(triggered()), this, SLOT(onTrayMenuCloseTriggered()));
    //设置风格
    //QString mainStyle("border:1px solid #FF0000;background:rgba(0, 0, 0,100%);color:white;");
    //setStyleSheet("border:1px solid #FF0000;background:rgba(0, 0, 0,100%);color:white;");
    //信号槽
    qRegisterMetaType<QSharedPointer<StockTask>>("QSharedPointer<StockTask>");
    connect(ui->kmap, SIGNAL(selectK(KData*)), this, SLOT(on_k_select(KData*)) );
    connect(StockMgr::single(), SIGNAL(sendTaskFinished(QSharedPointer<StockTask>)), this, SLOT(onTaskFinished(QSharedPointer<StockTask>)), Qt::ConnectionType::QueuedConnection);
    connect(StockMgr::single(), SIGNAL(sendSyncProgress(float)), this, SLOT(onSyncProgress(float)), Qt::ConnectionType::QueuedConnection);
    connect(StockMgr::single(), SIGNAL(sendMessage(int, QString)), this, SLOT(onMessage(int, QString)), Qt::ConnectionType::QueuedConnection);
    connect(ui->stockTable->horizontalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(onTableSort(int)));//点击表头排序
    //数据初始化
    StockMgr* sm = StockMgr::single();
    QString err = sm->init();
    if(err!=nullptr)
    {
        QMessageBox::critical(nullptr, "错误", err, QMessageBox::Cancel);
    }
    //显示股票列表
    sorts = new Qt::SortOrder[ui->stockTable->columnCount()];
    const QList<Stock*>& ls = sm->getStocks();
    ui->stockTable-> setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
    ui->stockTable->setSelectionMode(QAbstractItemView::SingleSelection);//单选
    ui->stockTable->setSelectionBehavior(QAbstractItemView::SelectRows);//单击格也选中一行
    ui->stockTable->setContextMenuPolicy(Qt::CustomContextMenu);//不设置弹不出菜单
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
        ui->stockTable->setItem(i,5,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,6,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,7,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,8,new QTableWidgetItem("-"));
        QTableWidgetItem* it =  new QTableWidgetItem(sk->blacklist?"√":"");
        it->setTextAlignment(Qt::AlignCenter);
        ui->stockTable->setItem(i,9,it);
    }
    init();
}

MainWindow::~MainWindow()
{
    mSysTrayIcon->hide();
    delete  mSysTrayIcon;
    StockMgr::single()->deinit();
    delete ui;
}

QString MainWindow::toMoney(double money)
{
    if(money>100000000)return QString::number(money/100000000,'f',2)+"亿";
    if(money>10000)return QString::number(money/10000,'f',2)+"万";
    return QString::number(money,'f',2);
}

void MainWindow::on_k_select(KData* k)
{
    if(k!=nullptr)
    {
        QDate date = QDate::fromString(StockMgr::int2StrTime(k->date),"yyyy-MM-dd");
        QString ds = date.toString("yy/MM/dd/dddd").remove(9,2);
        ui->lbDate->setText(ds);
        ui->lbOpen->setText(QString::asprintf("开 %0.2f",k->open));
        ui->lbClose->setText(QString::asprintf("收 %0.2f",k->close));
        ui->lbHigh->setText(QString::asprintf("高 %0.2f",k->high));
        ui->lbLow->setText(QString::asprintf("低 %0.2f",k->low));
        ui->lbAVG->setText(QString::asprintf("均 %0.2f",k->avg));
        ui->lbVolume->setText(QString::asprintf("量 %0.2f",k->volume));
        ui->lbAmount->setText(QString("额 ")+toMoney(k->amount));
        ui->lbChg->setText(QString::asprintf("涨 %0.2f",k->change));
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
        ui->lbAVG->setText("");
    }
}

void MainWindow::on_toolSync_triggered()
{
    LoadingDlg::showLoading(this);
    StockMgr::single()->syncData();
}

void MainWindow::on_toolAnalyse_triggered()
{
    StockMgr::single()->analyseData();
}

void MainWindow::on_toolSave_triggered()
{
    StockMgr::single()->saveData();
}

void  MainWindow::on_toolBlacklist_triggered()
{
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    for(int i=0;i<stocks.length();++i)
    {
        Stock* sk = stocks[i];
        if( sk->name.startsWith("S")||sk->name.startsWith('*'))
        {
            sk->blacklist = true;
            sk->setDirty();
            QList<QTableWidgetItem*> ls = ui->stockTable->findItems(sk->code,Qt::MatchExactly);
            if(ls.size()<1)return;
            ui->stockTable->item(ls[0]->row(),9)->setText("√");
        }
    }
}

void MainWindow::on_toolSafeSync_triggered(bool checked)
{
    if(checked)
    {
        timerID = startTimer(2000);
    }
    else
    {
        if(timerID!=0)killTimer(timerID);
        timerID=0;
    }
}

void  MainWindow::on_tookCheck_triggered()
{
    StockMgr::single()->checkData();
}

void MainWindow::on_menuSync_triggered()
{
    StockMgr::single()->syncData(selectedStock);
}

void MainWindow::on_menuSave_triggered()
{
    StockMgr::single()->saveData();
}

void MainWindow::on_menuAnalyse_triggered()
{
    StockMgr::single()->analyseData(selectedStock);
}

void MainWindow::on_menuBlacklist_triggered(bool checked)
{
    selectedStock->blacklist = checked;
    selectedStock->setDirty();
    QList<QTableWidgetItem*> ls = ui->stockTable->findItems(selectedStock->code,Qt::MatchExactly);
    if(ls.size()<1)return;
    ui->stockTable->item(ls[0]->row(),9)->setText(checked?"√":"");
}

void MainWindow::on_menuReset_triggered()
{
     QMessageBox:: StandardButton r = QMessageBox::information(this,"tip","是否重置该股票数据",QMessageBox::Yes | QMessageBox::No);
     if(r == QMessageBox::Yes)selectedStock->reset();
}

void MainWindow::on_stockTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    QString code = ui->stockTable->item(currentRow,0)->text();
    selectedStock = StockMgr::single()->getStock(code,false);
    ui->kmap->setStock(selectedStock);
    ui->statusBar->showMessage(selectedStock->name);
}

void MainWindow::onTableSort(int col)
{
    if(col<0||col>=ui->stockTable->columnCount())return;
    Qt::SortOrder& order = sorts[col];
    order = order==Qt::AscendingOrder?Qt::DescendingOrder:Qt::AscendingOrder;
    ui->stockTable->sortItems(col,order);
}

void MainWindow::on_stockTable_customContextMenuRequested(const QPoint &pos)
{
    if(selectedStock==nullptr)return;
    ui->menuBlacklist->setChecked(selectedStock->blacklist);
    tableMenu->exec(QCursor::pos());
}

void MainWindow::onTaskFinished(QSharedPointer<StockTask> shareTask)
{
    StockTask* task = shareTask.get();
    if(task->mTaskFlag == TaskFlag::Analyse)
    {
        QList<QTableWidgetItem*> ls = ui->stockTable->findItems(task->mStock->code,Qt::MatchExactly);
        if(ls.size()<1)return;
        AnalyseInfo& ad = task->mStock->mAnalyseInfo;
        ui->stockTable->item(ls[0]->row(),7)->setText(QString().sprintf("%0.2f=%d/%d", ad.yearDown==0?366:1.0f*ad.yearUp/ad.yearDown, ad.yearUp, ad.yearDown));
        ui->stockTable->item(ls[0]->row(),8)->setText(QString().sprintf("%0.2f=%d/%d", ad.monthDown==0?31:1.0f*ad.monthUp/ad.monthDown, ad.monthUp, ad.monthDown));
         //if(StockMgr::single()->getTaskCount()<1)this->mSysTrayIcon->showMessage("tip","分析任务完成");//托盘气泡,弹出太多会卡顿
    }

    if(!ui->kmap->isShowing(task->mStock))return;
    ui->kmap->update();
}

void MainWindow::onSyncProgress(float progress)
{
    if(progress>=1)LoadingDlg::hideLoading();
    ui->progressBar->setValue(progress*100);
}

void MainWindow::onMessage(int type, QString msg)
{
    switch (type)
    {
    case MsgType::TaskCount:
        ui->statusBar->showMessage(QString().sprintf("后台任务数:%d",  StockMgr::single()->getTaskCount()));
        break;
    case MsgType::MessageBox:
        QMessageBox::information(this,"tip",msg);
        break;
    default:
        ui->statusBar->showMessage(msg);
        break;
    }
}


void MainWindow::init()
{
    //表格右键菜单
    tableMenu =  new QMenu(ui->stockTable);
    tableMenu->addAction(ui->menuSync);//在设计器里已添加的action
    tableMenu->addAction(ui->menuSave);
    tableMenu->addAction(ui->menuAnalyse);
    tableMenu->addAction(ui->menuBlacklist);
    tableMenu->addSeparator();
    tableMenu->addAction(ui->menuReset);
}

 bool MainWindow::event(QEvent *event)
 {
     QEvent::Type et = event->type();
     switch (et)
     {
        case QEvent::Resize:
        case QEvent::Move:
         {
             QPoint pt = this->pos();
             QSize sz = this->size();
             LoadingDlg::setPosition(pt+QPoint(0.5*sz.width(), 0.5*sz.height()));
             return true;
         }
         case QEvent::WindowStateChange:
             if(windowState() == Qt::WindowMinimized)
             {
                this->hide();
             }
             return true;
         default:
            return QWidget::event(event);
     }
 }

 void MainWindow::closeEvent(QCloseEvent * event)
 {//拦截关闭按钮，隐藏程序不退出
    event->ignore();
    this->hide();
 }

 void MainWindow::timerEvent(QTimerEvent *event)
 {
    if(event->timerId()!=timerID)return;
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    for(;syncStockIndex<stocks.length();++syncStockIndex)
    {
        Stock* sk = stocks[syncStockIndex];
        if(sk->blacklist)continue;
        if(StockMgr::single()->isBusy())return;
        StockMgr::single()->syncData(sk);
        if(StockMgr::single()->getTaskCount()<1)continue;
    }
 }

 void MainWindow::onActivatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason)
 {
     switch(reason)
     {
         case QSystemTrayIcon::Trigger://单击托盘图标
         case QSystemTrayIcon::DoubleClick: //双击托盘图标
             this->showNormal();//正常大小显示
             this->activateWindow();//显示到前面
             break;
         default:
             break;
         }
 }

 void MainWindow::onTrayMenuAutoStartTriggered(bool sel)
 {
     setAutoStart(sel);
 }

 void MainWindow::onTrayMenuCloseTriggered()
 {
     this->close();
     QApplication::quit();
 }

void MainWindow::setAutoStart(bool autoRun)
{
        const  char*  REG_RUN = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        QString applicationName = QApplication::applicationName();
        QSettings *settings = new QSettings(REG_RUN, QSettings::NativeFormat);
        if(autoRun)
        {
            QString application_path = QApplication::applicationFilePath();
            settings->setValue(applicationName, application_path.replace("/", "\\"));
        }
        else
        {
            settings->remove(applicationName);
        }
        delete settings;
}

bool MainWindow::getAutoStart()
{
    const  char*  REG_RUN = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    QString applicationName = QApplication::applicationName();
    QSettings settings(REG_RUN, QSettings::NativeFormat);
    QString path = settings.value(applicationName, "").toString();
    return path==QApplication::applicationFilePath().replace("/", "\\");
}

#pragma execution_character_set("utf-8")
#include <QtDebug>
#include <QDate>
#include <QScrollArea>
#include <QSettings>
#include <QMenu>
#include <QMovie>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "LoadingDlg.h"
#include "MyToolAction.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
  ,selectedStock(nullptr)
  ,syncTimer(0)
  ,syncIndex(0)
  ,checkTimer(0)
  ,checkIndex(0)
  ,analyseTimer(0)
  ,analyseIndex(0)
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
    connect(StockMgr::single(), SIGNAL(sendTaskStart(StockTask*)), this, SLOT(onTaskStart(StockTask*)), Qt::ConnectionType::QueuedConnection);
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
        ui->stockTable->setItem(i,TB::DM,new QTableWidgetItem(sk->code));
        ui->stockTable->setItem(i,TB::MC,new QTableWidgetItem(sk->name));
        ui->stockTable->setItem(i,TB::HY,new QTableWidgetItem(sk->industry));
        ui->stockTable->setItem(i,TB::DQ,new QTableWidgetItem(sk->area));
        ui->stockTable->setItem(i,TB::SH,new QTableWidgetItem(QString::number(sk->marketTime, 10)));
        ui->stockTable->setItem(i,TB::XJ,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,TB::ZF,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,TB::NZDB,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,TB::YZDB,new QTableWidgetItem("-"));
        QTableWidgetItem* it =  new QTableWidgetItem(sk->blacklist?"√":"");
        it->setTextAlignment(Qt::AlignCenter);
        ui->stockTable->setItem(i,TB::HMD,it);
        ui->stockTable->setItem(i,TB::ZT,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,TB::RZDB,new QTableWidgetItem("-"));
        ui->stockTable->setItem(i,TB::LZ,new QTableWidgetItem("-"));
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
        ui->lbVolume->setText(QString("量 ")+toMoney(k->volume));
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

void MainWindow::on_toolSave_triggered()
{
    Result ret = StockMgr::single()->saveData();
     if(ret!=Result::Ok)QMessageBox::information(this,"tip",StockMgr::Result2Msg(ret));
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
            ui->stockTable->item(ls[0]->row(),TB::HMD)->setText("√");
        }
    }
}

void MainWindow::on_toolQuickSync_triggered()
{
    Result ret = StockMgr::single()->syncToday();
    if(ret!=Result::Ok)QMessageBox::information(this,"tip",StockMgr::Result2Msg(ret));
}

void MainWindow::on_toolSafeSync_triggered(bool checked)
{
    if(checked)
    {
        syncTimer = startTimer(500);
    }
    else
    {
        if(syncTimer!=0)killTimer(syncTimer);
        syncTimer=0;
    }
}

void  MainWindow::on_tookCheck_triggered(bool checked)
{
    if(checked)
    {
        checkTimer = startTimer(10);
    }
    else
    {
        if(checkTimer!=0)killTimer(checkTimer);
        checkTimer=0;
    }
}

void MainWindow::on_toolAnalyse_triggered(bool checked)
{
    if(checked)
    {
        analyseTimer = startTimer(100);
    }
    else
    {
        if(analyseTimer!=0)killTimer(analyseTimer);
        analyseTimer=0;
    }
}

void MainWindow::on_toolRemove_triggered()
{
    qDebug()<<111;
    QString s = dateEdit->text();
    QRegExp regExp("^([1-9][0-9]{3})-((0([1-9]{1}))|(1[0-2]))-(([0-2]([1-9]{1}))|(3[0|1]))$");
    if(!regExp.exactMatch(s))
    {
        QMessageBox::information(this,"tip","日期格式错误");
        return;
    }

    int date =s.remove('-').toInt();
    QMessageBox:: StandardButton r = QMessageBox::information(this,"tip","是否删除所有股票"+QString().number(date)+"日及以后的历史数据",QMessageBox::Yes | QMessageBox::No);
    if(r == QMessageBox::Yes)StockMgr::single()->removeData(date);
}

void MainWindow::on_menuSync_triggered()
{
    Result ret = StockMgr::single()->syncData(selectedStock);
    if(ret!=Result::Ok)QMessageBox::information(this,"tip",StockMgr::Result2Msg(ret));
}

void MainWindow::on_menuCheck_triggered()
{
    Result ret = StockMgr::single()->checkData(selectedStock);
     if(ret!=Result::Ok)QMessageBox::information(this,"tip",StockMgr::Result2Msg(ret));
}

void MainWindow::on_menuAnalyse_triggered()
{
    Result ret = StockMgr::single()->analyseData(selectedStock);
     if(ret!=Result::Ok)QMessageBox::information(this,"tip",StockMgr::Result2Msg(ret));
}

void MainWindow::on_menuBlacklist_triggered(bool checked)
{
    selectedStock->blacklist = checked;
    selectedStock->setDirty();
    QList<QTableWidgetItem*> ls = ui->stockTable->findItems(selectedStock->code,Qt::MatchExactly);
    if(ls.size()<1)return;
    ui->stockTable->item(ls[0]->row(),TB::HMD)->setText(checked?"√":"");
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
    QString s = QString().sprintf("%s, 最新日期 %d 丢失日期 %d 错误日期 %d", selectedStock->name, selectedStock->mCheckInfo.firstDate, selectedStock->mCheckInfo.firstLoseDate, selectedStock->mCheckInfo.firstErrorDate);
    ui->statusBar->showMessage(s);
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

void MainWindow::onTaskStart(StockTask* task)
{
    switch(task->mTaskFlag)
    {
    case TaskFlag::SaveDB:
        savingLB->show();
        break;
    }
}

void MainWindow::onTaskFinished(QSharedPointer<StockTask> shareTask)
{
    StockTask* task = shareTask.get();
    switch(task->mTaskFlag)
    {
    case TaskFlag::Analyse:
        {
            QList<QTableWidgetItem*> ls = ui->stockTable->findItems(task->mStock->code,Qt::MatchExactly);
            if(ls.size()<1)break;
            AnalyseInfo& ad = task->mStock->mAnalyseInfo;
            if(ad.yearUp+ad.yearDown!=0)ui->stockTable->item(ls[0]->row(),TB::NZDB)->setText(QString().sprintf("%0.2f=%d/%d", ad.yearDown==0?366:1.0f*ad.yearUp/ad.yearDown, ad.yearUp, ad.yearDown));
            if(ad.monthDown+ad.monthDown!=0)ui->stockTable->item(ls[0]->row(),TB::YZDB)->setText(QString().sprintf("%0.2f=%d/%d", ad.monthDown==0?31:1.0f*ad.monthUp/ad.monthDown, ad.monthUp, ad.monthDown));
            if(ad.sevenUp+ad.sevenDown!=0)ui->stockTable->item(ls[0]->row(),TB::RZDB)->setText(QString().sprintf("%0.2f=%d/%d", ad.sevenDown==0?7:1.0f*ad.sevenUp/ad.sevenDown, ad.sevenUp, ad.sevenDown));
            if(ad.continueRiseDay>0)ui->stockTable->item(ls[0]->row(),TB::LZ)->setText(QString().sprintf("%d,%0.2f", ad.continueRiseDay, ad.continueRiseRate));
            break;
        }
    case TaskFlag::InitList:
        saveTimer = startTimer(30000);//五分钟写一次数据库
        break;
    case TaskFlag::InitMaxDate:
        maxDateLB->setText(QString().number(StockMgr::single()->getRefStock()->maxDate));
        break;
    case TaskFlag::InitMinDate:
        minDateLB->setText(QString().number(StockMgr::single()->getRefStock()->minDate));
        break;
    case TaskFlag::SaveDB:
        savingLB->hide();
        break;
    case TaskFlag::Check:
        {
            QList<QTableWidgetItem*> ls = ui->stockTable->findItems(task->mStock->code,Qt::MatchExactly);
            if(ls.size()<1)break;
            QString s;
            if((task->mStock->mCheckInfo.state&StockState::DataNotNew)!=0)s.append("旧");
            if((task->mStock->mCheckInfo.state&StockState::DataLose)!=0)s.append("缺");
            if((task->mStock->mCheckInfo.state&StockState::DataError)!=0)s.append("错");
            ui->stockTable->item(ls[0]->row(),TB::ZT)->setText(s);
        }
        break;
    }

    if(!ui->kmap->isShowing(task->mStock))return;
    ui->kmap->update();
}

void MainWindow::onSyncProgress(float progress)
{
    if(progress>=1)LoadingDlg::hideLoading();
    progressBar->setValue(progress*100);
}

void MainWindow::onMessage(int type, QString msg)
{
    switch (type)
    {
    case MsgType::TaskCount:
        taskCountLB->setText(QString().sprintf("后台任务数:%d",  StockMgr::single()->getTaskCount()));
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
    tableMenu->addAction(ui->menuCheck);
    tableMenu->addAction(ui->menuAnalyse);
    tableMenu->addAction(ui->menuBlacklist);
    tableMenu->addSeparator();
    tableMenu->addAction(ui->menuReset);
    //工具栏(图片下显示文字)
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolBar->insertSeparator(ui->space);
    ui->toolBar->insertWidget(ui->space, actionCheck = new MyToolAction(ui->tookCheck,ui->toolBar));
    ui->toolBar->insertSeparator(ui->space);
    ui->toolBar->insertWidget(ui->space, actionAnalyse = new MyToolAction(ui->toolAnalyse,ui->toolBar));
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(actionSafeSync = new MyToolAction(ui->toolSafeSync,ui->toolBar));
    ui->toolBar->addSeparator();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(dateEdit = new QLineEdit());
    QDate now = QDate::currentDate();
    dateEdit->setText(QString().sprintf("%d-%02d-%02d", now.year(), now.month(), now.day()));
    QToolButton* bt = new QToolButton(this);
    bt->setText("删除");
    bt->setDefaultAction(ui->toolRemove);
    bt->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    layout->addWidget(bt);
    layout->setMargin(2);
    layout->setSpacing(1);
    QWidget* w = new QWidget();
    w->setFixedWidth(72);
    w->setLayout(layout);
    ui->toolBar->addWidget(w);
    ui->toolBar->addSeparator();
    connect(actionCheck, SIGNAL(sendReset()), this, SLOT(onCheckReset()));
    connect(actionAnalyse, SIGNAL(sendReset()), this, SLOT(onAnalyseReset()));
    connect(actionSafeSync, SIGNAL(sendReset()), this, SLOT(onSafeSyncReset()));
    //状态栏初始化(无法通过设计器添加)
    ui->statusBar->addPermanentWidget(progressBar=new QProgressBar());
    ui->statusBar->addPermanentWidget(savingLB=new QLabel(""));
    ui->statusBar->addPermanentWidget(taskCountLB=new QLabel("后台任务数:0"));
    ui->statusBar->addPermanentWidget(minDateLB=new QLabel("0"));
    ui->statusBar->addPermanentWidget(maxDateLB=new QLabel("0"));
    progressBar->setRange(0,100);
    progressBar->setValue(0);
    progressBar->setFixedSize(120,24);
    progressBar->setTextVisible(false);
    progressBar->setOrientation(Qt::Horizontal);
    savingLB->setMaximumSize(24,24);
    savingLB->setMinimumSize(24,24);
    QMovie *movie = new QMovie(":/gif/Res/saving.gif");
    savingLB->setMovie(movie);
    movie->setScaledSize(savingLB->size());
    movie->start();
    savingLB->hide();
    taskCountLB->setMinimumSize(128,24);
    taskCountLB->setFrameShape(QFrame::Panel);
    taskCountLB->setFrameShadow(QFrame::Sunken);
    minDateLB->setMinimumSize(64,24);
    minDateLB->setFrameShape(QFrame::Panel);
    minDateLB->setFrameShadow(QFrame::Sunken);
    maxDateLB->setMinimumSize(64,24);
    maxDateLB->setFrameShape(QFrame::Panel);
    maxDateLB->setFrameShadow(QFrame::Sunken);
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
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    int count = stocks.length();
    int timeId = event->timerId();
    if(timeId==analyseTimer)
    {//分析
        for(;analyseIndex<count;++analyseIndex)
        {
            Stock* sk = stocks[analyseIndex];
            if(sk->blacklist)continue;
            if(StockMgr::single()->analyseData(sk)==Result::Ok)++analyseIndex;
            actionAnalyse->setProgress(1.0f*analyseIndex/count);
            return;
        }
    }
    else if(timeId==syncTimer)
    {//同步
        for(;syncIndex<count;++syncIndex)
        {
            Stock* sk = stocks[syncIndex];
            if(sk->blacklist)continue;
            if(StockMgr::single()->syncData(sk)==Result::Ok)++syncIndex;
            actionSafeSync->setProgress(1.0f*syncIndex/count);
            return;
        }
    }
    else if(timeId==checkTimer)
    {//检测
        for(;checkIndex<count;++checkIndex)
        {
            Stock* sk = stocks[checkIndex];
            if(sk->blacklist)continue;
            if(StockMgr::single()->checkData(sk)==Result::Ok)++checkIndex;
            actionCheck->setProgress(1.0f*checkIndex/count);
            return;
        }
    }
    else if(timeId==saveTimer)
    {//存储
        StockMgr::single()->saveData();
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

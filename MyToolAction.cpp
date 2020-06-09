#pragma execution_character_set("utf-8")
#include <QPainter>
#include <QDebug>
#include "MyToolAction.h"

MyToolAction::MyToolAction(QAction* action, QWidget *parent) :QToolButton(parent),mProgress(0)
{
    setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    setCheckable(action->isCheckable());
    setChecked(action->isChecked());
    setIcon(action->icon());
    setText(action->text());
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    setDefaultAction(action);//关联信号

    setContextMenuPolicy (Qt::CustomContextMenu);
    menu = new QMenu(this);
    QAction* a = new QAction("重置",this);
    connect(a,SIGNAL(triggered()),this,SLOT(onReset()));
    menu->addAction(a);
    setMenu(menu);
    setPopupMode(ToolButtonPopupMode::MenuButtonPopup);
}

 void MyToolAction::paintEvent(QPaintEvent * event)
 {
    this->QToolButton::paintEvent(event);
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::SolidPattern);
    painter.setBrush(QColor(0,160,0,100));
    int w = this->width();
    int h = this->height();
    painter.drawPie(-w/2-h/2+w/2,-w/2-h/2+h/2,w+h,w+h, 90*16, -mProgress*360*16);//角度单位为1/16度
 }

 void MyToolAction::showMenu(const QPoint pos)
 {
    menu->exec(mapToGlobal(pos));
 }

 void MyToolAction::onReset()
 {
    setProgress(0);
    this->defaultAction()->setChecked(false);
    this->defaultAction()->triggered(false);
    emit sendReset();
 }

#include "stdafx.h"
#include "listitem.h"
#include "ui_listitem.h"

ListItem::ListItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ListItem)
{
    ui->setupUi(this);
}

ListItem::~ListItem()
{
    delete ui;
}

void ListItem::on_pushButton_clicked()
{
    QListWidgetItem* item = (QListWidgetItem*)li;
    item->listWidget()->removeItemWidget(item);
    delete  item;
}

void ListItem::init(int idx, Stock* stock)
{
    mStock = stock;
    ui->index->setText(QString::number(idx,10));
    ui->code->setText(stock->code);
    ui->name->setText(stock->name);
}

void ListItem::on_freshBt_clicked()
{
    emit reqStock(mStock->code);
}

#ifndef LISTITEM_H
#define LISTITEM_H

#include <QWidget>
#include <QtWidgets/QListWidget>
#include "AraleStock.h"

namespace Ui {
class ListItem;
}

class ListItem : public QWidget
{
    Q_OBJECT

public:
    explicit ListItem(QWidget *parent = nullptr);
    ~ListItem();

    QListWidgetItem* li;
private slots:
    void on_pushButton_clicked();
    void on_freshBt_clicked();
signals:
    void reqStock(QString code);
private:
    Ui::ListItem *ui;
    Stock* mStock;
public:
    void init(int idx, Stock* stock);
};

#endif // LISTITEM_H

#ifndef MYTOOLACTION_H
#define MYTOOLACTION_H
#include <QToolButton>
#include <QAction>
#include <QMenu>

class MyToolAction : public QToolButton
{
    Q_OBJECT

public:
    explicit MyToolAction(QAction* action, QWidget *parent = 0);

    void setProgress(float val){mProgress=val;this->update();}
protected:
    QMenu *menu;
    float mProgress;

    void paintEvent(QPaintEvent * event);


private slots:
    void showMenu(const QPoint pos);
    void onReset();
signals:
    void sendReset();
};

#endif // MYTOOLACTION_H

#ifndef LOADINGDLG_H
#define LOADINGDLG_H

#include <QDialog>

namespace Ui {
class LoadingDlg;
}

class LoadingDlg : public QDialog
{
    Q_OBJECT

    static LoadingDlg* mDlg;
public:
    explicit LoadingDlg(QWidget *parent = nullptr);
    ~LoadingDlg();

private:
    Ui::LoadingDlg *ui;

public:
    static void showLoading(QWidget* parent);
    static void hideLoading();
    static void setPosition(QPoint pt);
};

#endif // LOADINGDLG_H

#include "LoadingDlg.h"
#include "ui_LoadingDlg.h"
#include <QMovie>
#include <QDebug>

LoadingDlg* LoadingDlg::mDlg=nullptr;

LoadingDlg::LoadingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadingDlg)
{
    ui->setupUi(this);
    Qt::WindowFlags m_flags = windowFlags();
    setWindowFlags(m_flags|Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);//背景透明


    QMovie *movie = new QMovie(":/gif/Res/loading.gif");
    ui->anim->setMovie(movie);
    movie->start();
    ui->anim->show();
}

LoadingDlg::~LoadingDlg()
{
    delete ui;
}

void LoadingDlg::showLoading(QWidget* parent)
{
    if(mDlg==nullptr)
    {
        mDlg = new LoadingDlg(parent);
        //mDlg->setModal(true);
    }
    mDlg->show();
}

void LoadingDlg::hideLoading()
{
     if(mDlg==nullptr)return;
    mDlg->hide();
}

void LoadingDlg::setPosition(QPoint pt)
{
    if(mDlg==nullptr)return;
    QSize sz = mDlg->size();
    mDlg->move(pt+QPoint(-0.5*sz.width(), -0.5*sz.height()));
}

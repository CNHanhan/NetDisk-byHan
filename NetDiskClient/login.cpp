#include "login.h"
#include "ui_login.h"
#include "pack.h"
#include <QMessageBox>

Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    m_pKernel = 0;
    setWindowTitle("登录");
    resize(400,400);
    setWindowIcon(QIcon(":/netdisk.png"));
}

Login::~Login()
{
    delete ui;
}

void Login::setKernel(IKernel *pKernel)
{
    m_pKernel = pKernel;
}

void Login::on_reg_clicked()
{
    // 获取注册页面的文本信息，构造为注册请求包
    QString user = ui->reg_user->text();
    QString pwd = ui->reg_pwd->text();
    QString tel = ui->reg_tel->text();
    STRU_REG_RQ srr;
    srr.m_tel = atoll(tel.toStdString().c_str());
    strcpy(srr.m_user, user.toStdString().c_str());
    strcpy(srr.m_pwd, pwd.toStdString().c_str());
    m_pKernel->sendData((char*)&srr, sizeof(srr));

}

void Login::slots_regrs(STRU_REG_RS * psrr)
{
    if(psrr->m_ret == REG_SUC)
        QMessageBox::information(this,"提示","注册成功！");
    else
        QMessageBox::critical(this,"错误","注册失败！");
}


void Login::on_login_clicked()
{
    QString user = ui->login_user->text();
    QString pwd = ui->login_pwd->text();
    STRU_LOGIN_RQ slr;
    strcpy(slr.m_user, user.toStdString().c_str());
    strcpy(slr.m_pwd, pwd.toStdString().c_str());
    m_pKernel->sendData((char*)&slr, sizeof(slr));
}

QString Login::getLoginUserName() const
{
    return ui->login_user->text();
}

#include "login.h"
#include "ui_login.h"
#include "pack.h"
#include <QMessageBox>
#include <QTabBar>

Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    m_pKernel = 0;
    setWindowTitle("个人云网盘 - 登录");
    resize(340, 260);
    setWindowIcon(QIcon(":/netdisk.png"));

    // 隐藏 QTabWidget 自带的标签栏，改用页面底部的“注册 / 返回登录”入口切换
    ui->tabWidget->tabBar()->setVisible(false);
    ui->tabWidget->setCurrentIndex(0);      // 默认显示登录页

    ui->login_pwd->setEchoMode(QLineEdit::Password);
    ui->reg_pwd->setEchoMode(QLineEdit::Password);
}

Login::~Login()
{
    delete ui;
}

void Login::setKernel(IKernel *pKernel)
{
    m_pKernel = pKernel;
}

QString Login::getLoginUserName() const
{
    return ui->login_user->text().trimmed();
}

// 从登录页切到注册页
void Login::on_btn_goRegister_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
    ui->reg_user->setFocus();
}

// 从注册页切回登录页
void Login::on_btn_goLogin_clicked()
{
    ui->tabWidget->setCurrentIndex(0);
    ui->login_user->setFocus();
}

void Login::on_reg_clicked()
{
    QString user = ui->reg_user->text().trimmed();
    QString pwd  = ui->reg_pwd->text();
    QString tel  = ui->reg_tel->text().trimmed();

    if (user.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入用户名");
        return;
    }
    if (user.length() > MAXSIZE - 1)
    {
        QMessageBox::warning(this, "提示", QString("用户名不能超过 %1 个字符").arg(MAXSIZE - 1));
        return;
    }
    if (pwd.length() < 4)
    {
        QMessageBox::warning(this, "提示", "密码至少 4 位");
        return;
    }
    if (pwd.length() > MAXSIZE - 1)
    {
        QMessageBox::warning(this, "提示", QString("密码不能超过 %1 个字符").arg(MAXSIZE - 1));
        return;
    }
    if (tel.length() != 11)
    {
        QMessageBox::warning(this, "提示", "手机号必须为 11 位数字");
        return;
    }
    for (int i = 0; i < tel.length(); i++)
    {
        if (!tel.at(i).isDigit())
        {
            QMessageBox::warning(this, "提示", "手机号只能包含数字");
            return;
        }
    }

    STRU_REG_RQ srr;
    srr.m_tel = atoll(tel.toStdString().c_str());
    strncpy(srr.m_user, user.toUtf8().constData(), MAXSIZE - 1);
    srr.m_user[MAXSIZE - 1] = '\0';
    strncpy(srr.m_pwd, pwd.toUtf8().constData(), MAXSIZE - 1);
    srr.m_pwd[MAXSIZE - 1] = '\0';
    m_pKernel->sendData((char*)&srr, sizeof(srr));
}

void Login::slots_regrs(STRU_REG_RS * psrr)
{
    if(psrr->m_ret == REG_SUC)
    {
        QMessageBox::information(this,"提示","注册成功，请登录！");
        // 注册成功后自动切回登录页并填好用户名
        ui->login_user->setText(ui->reg_user->text().trimmed());
        ui->login_pwd->clear();
        ui->tabWidget->setCurrentIndex(0);
        ui->login_pwd->setFocus();
    }
    else
    {
        QMessageBox::critical(this,"错误","注册失败：用户名或手机号已被注册");
    }
}

void Login::on_login_clicked()
{
    QString user = ui->login_user->text().trimmed();
    QString pwd  = ui->login_pwd->text();

    if (user.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入用户名");
        return;
    }
    if (pwd.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入密码");
        return;
    }

    STRU_LOGIN_RQ slr;
    strncpy(slr.m_user, user.toUtf8().constData(), MAXSIZE - 1);
    slr.m_user[MAXSIZE - 1] = '\0';
    strncpy(slr.m_pwd, pwd.toUtf8().constData(), MAXSIZE - 1);
    slr.m_pwd[MAXSIZE - 1] = '\0';
    m_pKernel->sendData((char*)&slr, sizeof(slr));
}

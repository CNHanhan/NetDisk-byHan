#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <kernel/IKernel.h>
#include "pack.h"

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();
    void setKernel(IKernel* pKernel);
    QString getLoginUserName() const;


private slots:
    void on_reg_clicked();

    void on_login_clicked();

public slots:
    void slots_regrs(STRU_REG_RS*);

private:
    Ui::Login *ui;
    IKernel* m_pKernel;
};

#endif // LOGIN_H

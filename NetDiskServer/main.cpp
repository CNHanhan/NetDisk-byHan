#include <QCoreApplication>
#include "kernel/tcpkernel.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    IKernel* p = TCPKernel::getKernel();
    if(p->open())
    {
        qDebug() <<"server is runing....";
    }
    else {
        qDebug() <<"server is err....";
    }


    return a.exec();
}

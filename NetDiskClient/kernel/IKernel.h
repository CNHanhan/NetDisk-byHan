#ifndef IKERNEL_H
#define IKERNEL_H
#include <winsock2.h>

class IKernel
{
public:
    IKernel(){}
    virtual ~IKernel(){}
    // 初始化网络
    virtual bool connectServer(const char* ip = "127.0.0.1", u_short port = 8899) = 0;
    // 卸载网络
    virtual void disconnectServer(const char* errmsg) = 0;
    // 处理数据
    virtual void dealData(char* buf) = 0;
    // 发送数据
    virtual bool sendData(char* buf, int bufLen) = 0;
};

#endif // IKERNEL_H

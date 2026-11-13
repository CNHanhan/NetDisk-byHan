#ifndef INET_H
#define INET_H
#include <winsock2.h>

class INet
{
public:

    INet(){}
    virtual ~INet(){}
    // 初始化网络
    virtual bool InitNetwork(const char* ip = "127.0.0.1", u_short port = 8899) = 0;
    // 卸载网络
    virtual void UnInitNetwork(const char* errmsg) = 0;
    // 接收数据
    virtual bool RecvData() = 0;
    // 发送数据
    virtual bool SendData(char* buf, int bufLen) = 0;
};

#endif // INET_H

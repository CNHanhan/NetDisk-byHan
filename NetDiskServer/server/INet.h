#ifndef INET_H
#define INET_H
#include <winsock2.h>

class INet
{
public:
    INet(){}
    virtual ~INet(){}
public:
    // 服务器网络初始化
    virtual bool InitNetwork(char* ip = (char*)"127.0.0.1", u_short port = 8899) = 0;   // 加载库，创建套接字，绑定端口，监听
    // 接收数据
    virtual bool RecvData(SOCKET sockWaiter) = 0;     // 通过处理粘包的方式接收数据
    // 发送数据
    virtual bool SendData(SOCKET sockWaiter, char* buf, int bufLen) = 0;
    // 卸载网络
    virtual void UninitNetwork(char* errmsg) = 0;
};


#endif // INET_H

#ifndef INET_H
#define INET_H

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SOCKET;
typedef unsigned short u_short;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
// 加 :: 前缀：避免在含 close() 成员函数的类里被解析成成员函数（名字隐藏）
#define closesocket(s) ::close(s)
#endif

class INet
{
public:
    INet(){}
    virtual ~INet(){}
public:
    // 服务器网络初始化（默认监听 0.0.0.0，云服务器上外部才能连进来）
    virtual bool InitNetwork(char* ip = (char*)"0.0.0.0", u_short port = 8899) = 0;
    // 接收数据
    virtual bool RecvData(SOCKET sockWaiter) = 0;
    // 发送数据
    virtual bool SendData(SOCKET sockWaiter, char* buf, int bufLen) = 0;
    // 卸载网络
    virtual void UninitNetwork(char* errmsg) = 0;
};

#endif // INET_H

#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "INet.h"
#include <thread>

class TCPServer : public INet
{
public:
    TCPServer();
    ~TCPServer();
public:
    // 服务器网络初始化（加载库、创建套接字、绑定端口、监听）
    bool InitNetwork(char* ip = (char*)"0.0.0.0", u_short port = 8899);
    // 接收数据（处理粘包/半包）
    bool RecvData(SOCKET sockWaiter);
    // 发送数据
    bool SendData(SOCKET sockWaiter, char* buf, int bufLen);
    // 卸载网络
    void UninitNetwork(char* errmsg);

private:
    SOCKET m_SOCK;          // 本地套接字
    static void ThreadProc(TCPServer* pThis , SOCKET sockWaiter); // 处理数据线程
    static void ThreadAccept(TCPServer* pThis);     // 接收连接线程
};

#endif // TCPSERVER_H

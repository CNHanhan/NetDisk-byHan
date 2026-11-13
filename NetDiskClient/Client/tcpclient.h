#ifndef TCPCLIENT_H
#define TCPCLIENT_H
#include "INet.h"
#include "../kernel/IKernel.h"



class TCPClient : public INet
{
public:
    TCPClient(IKernel* pKernel);
    ~TCPClient();
public:
    // 初始化网络
    bool InitNetwork(const char* ip = "127.0.0.1", u_short port = 8899);
    // 卸载网络
    void UnInitNetwork(const char* errmsg);
    // 接收数据
    bool RecvData();
    // 发送数据
    bool SendData(char* buf, int bufLen);
private:
    SOCKET m_SOCK;          // 本地套接字
    static void ThreadRecv(TCPClient* pThis); // 处理数据线程
    IKernel* m_pKernel;
};

#endif // TCPCLIENT_H

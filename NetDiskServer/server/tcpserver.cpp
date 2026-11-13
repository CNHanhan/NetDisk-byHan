#include "tcpserver.h"
#include "../kernel/tcpkernel.h"

TCPServer::TCPServer() {
    m_SOCK = 0;
}

TCPServer::~TCPServer()
{

}

bool TCPServer::InitNetwork(char *ip, u_short port)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        UninitNetwork((char*)"WSAStartup failed");
        return false;
    }


    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        UninitNetwork((char*)"Could not find a usable version of Winsock.dll");
        return false;
    }
    else
        printf("The Winsock 2.2 dll was found okay\n");



    // 创建套接字
    m_SOCK =  socket(AF_INET, SOCK_STREAM, 0);
    if (m_SOCK == INVALID_SOCKET)
    {
        UninitNetwork((char*)"socket err");
        return false;
    }

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.S_un.S_addr = inet_addr(ip);
    if(bind(m_SOCK, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
    {
        UninitNetwork((char*)"bind err");
        return false;
    }

    if(listen(m_SOCK, 128) == SOCKET_ERROR)
    {
        UninitNetwork((char*)"listen err");
        return false;
    }
    // printf("%p\n", this);
    std::thread td(TCPServer::ThreadAccept, this);
    td.detach();
    return true;
}

bool TCPServer::RecvData(SOCKET sockWaiter)
{
    int len = 0;    // 记录包长度
    int num = recv(sockWaiter, (char*)&len,sizeof(int), 0);
    if (num <= 0)
    {
        return false;
    }

    // 在堆区开辟空间
    char* szbuf = new char[len];    // 接收缓冲区
    // 在接收包内容
    int offset = 0;
    while(len)
    {
        num = recv(sockWaiter, szbuf + offset,len, 0);
        if (num > 0)
        {
            offset += num;
            len -= num;
        }
        else
        {
            delete[] szbuf;
            return false;
        }
    }
    // 处理数据
    // printf("Client Say : %s\n", szbuf);   // 打印接收的数据
    TCPKernel::getKernel()->dealData(sockWaiter, szbuf);
    delete[] szbuf;
    return true;
}

bool TCPServer::SendData(SOCKET sockWaiter, char *buf, int bufLen)
{
    // 第一次发送长度
    if (send(sockWaiter, (char*)&bufLen, sizeof(bufLen), 0) <= 0)
    {
        return false;
    }
    // 第二次发送数据
    if (send(sockWaiter, buf, bufLen, 0) <= 0)
    {
        return false;
    }
    return true;
}

void TCPServer::UninitNetwork(char *errmsg)
{
    printf("%s\n", errmsg);
    if(m_SOCK != 0)
    {
        closesocket(m_SOCK);
        m_SOCK = 0;
    }
    WSACleanup();
}

void TCPServer::ThreadProc(TCPServer* pThis, SOCKET sockWaiter)
{
    while(1)
    {
        if(!pThis->RecvData(sockWaiter))    // 如果当前接收数据失败，则退出线程
            break;
    }
    closesocket(sockWaiter);
}

void TCPServer::ThreadAccept(TCPServer *pThis)
{
    // printf("%p\n", pThis);
    sockaddr_in clientAddr;
    int size = sizeof(clientAddr);
    while(1)
    {
        SOCKET sockwaiter = accept(pThis->m_SOCK, (sockaddr*)&clientAddr, &size);
        printf("Client %s : %d connect!!!\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));

        // 如果有连接——继续创建线程，用来和客户端通信
        std::thread td(TCPServer::ThreadProc, pThis, sockwaiter);
        td.detach();
    }
    closesocket(pThis->m_SOCK);      // 关闭套接字
    pThis->m_SOCK = 0;
    WSACleanup();           // 卸载网络库
}









#include "tcpclient.h"
#include <thread>
#include <stdio.h>

TCPClient::TCPClient(IKernel* pKernel) {
    m_SOCK = 0;
    m_pKernel = pKernel;
}

TCPClient::~TCPClient()
{

}

bool TCPClient::InitNetwork(const char *ip, u_short port)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return false;
    }


    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        UnInitNetwork("Could not find a usable version of Winsock.dll\n");
        return false;
    }
    else
        printf("The Winsock 2.2 dll was found okay\n");



    // 创建套接字
    m_SOCK =  socket(AF_INET, SOCK_STREAM, 0);
    if (m_SOCK == INVALID_SOCKET)
    {
        UnInitNetwork("socket err");
        return false;
    }

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.S_un.S_addr = inet_addr(ip);

    if(connect(m_SOCK, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
    {
        UnInitNetwork("connect err");
        return false;
    }


    printf("connect Server\n");
    // 创建接收数据线程
    std::thread td(ThreadRecv, this);
    td.detach();
    return true;
}

void TCPClient::UnInitNetwork(const char *errmsg)
{
    printf("%s\n", errmsg);
    if(m_SOCK)
    {
        closesocket(m_SOCK);
        m_SOCK = 0;
    }
    WSACleanup();
}

bool TCPClient::RecvData()
{
    // 先接收包大小
    int len = 0;
    int num = recv(m_SOCK, (char*)&len,sizeof(int), 0);

    if (num > 0)
    {
        // 根据包大小在堆区开辟空间
        char* szbuf = new char[len];
        int offset = 0;
        while(len)
        {
            num = recv(m_SOCK, szbuf + offset, len, 0);
            if (num > 0)
            {
                offset += num;
                len -= num;
            }
            else
            {
                delete[] szbuf;
                UnInitNetwork("recv err");
                break;
            }
        }
        // 把数据交给kernel处理 todo
        m_pKernel->dealData(szbuf);
        delete[] szbuf;
        return true;
    }
    else
    {
        UnInitNetwork("recv err");
        return false;
    }
}

bool TCPClient::SendData(char *buf, int bufLen)
{
    // 第一次发送数据长度
    if(send(m_SOCK, (char*)&bufLen, sizeof(int), 0) <= 0) return false;
    // 发送真实数据
    if(send(m_SOCK, buf, bufLen, 0) <= 0) return false;
    return true;
}

void TCPClient::ThreadRecv(TCPClient *pThis)
{
    while(1)
    {
        if(!pThis->RecvData())
            break;
    }
    closesocket(pThis->m_SOCK);
    pThis->m_SOCK = 0;
}



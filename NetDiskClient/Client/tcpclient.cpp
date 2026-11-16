#include "tcpclient.h"
#include <thread>
#include <stdio.h>
#include <cstring>
#include <ws2tcpip.h>   // TCP_NODELAY

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

    // 关闭 Nagle 算法：否则每个 4KB 分包前那个 4 字节长度头都要等上一个包的 ACK，
    // 高延迟链路上吞吐量会被压到 4096/RTT（200ms 就只剩下 20KB/s）
    int nodelay = 1;
    setsockopt(m_SOCK, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

    // 加大收发缓冲区，减少 send 阻塞等待
    int bufSize = 256 * 1024;
    setsockopt(m_SOCK, SOL_SOCKET, SO_SNDBUF, (const char*)&bufSize, sizeof(bufSize));
    setsockopt(m_SOCK, SOL_SOCKET, SO_RCVBUF, (const char*)&bufSize, sizeof(bufSize));


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
    // 1. 循环读满 4 字节包头（recv 可能只返回 1~3 字节，必须循环补齐）
    int len = 0;
    int offset = 0;
    int left = sizeof(int);
    while (left > 0)
    {
        int num = recv(m_SOCK, ((char*)&len) + offset, left, 0);
        if (num <= 0)
        {
            UnInitNetwork("recv err");
            return false;
        }
        offset += num;
        left -= num;
    }

    // 2. 校验包长度，防止垃圾长度导致 std::bad_alloc 闪退
    if (len <= 0 || len > 1024 * 1024)
    {
        printf("invalid packet len: %d\n", len);
        return false;
    }

    // 3. 按长度读满整个包
    char* szbuf = new char[len];
    offset = 0;
    left = len;
    while (left > 0)
    {
        int num = recv(m_SOCK, szbuf + offset, left, 0);
        if (num <= 0)
        {
            delete[] szbuf;
            UnInitNetwork("recv err");
            return false;
        }
        offset += num;
        left -= num;
    }

    // 4. 交给 kernel 处理
    m_pKernel->dealData(szbuf);
    delete[] szbuf;
    return true;
}

bool TCPClient::SendData(char *buf, int bufLen)
{
    // 把 4 字节长度头和包体拼成一次 send 发出：
    // 分两次 write（小包 + 大包）会触发 Nagle 与延迟 ACK 互相等待，吞吐量暴跌
    size_t total = sizeof(int) + (size_t)bufLen;
    if (m_sendBuf.size() < total)
        m_sendBuf.resize(total);

    memcpy(m_sendBuf.data(), &bufLen, sizeof(int));
    memcpy(m_sendBuf.data() + sizeof(int), buf, bufLen);

    int offset = 0;
    int left = (int)total;
    while (left > 0)
    {
        int sent = send(m_SOCK, m_sendBuf.data() + offset, left, 0);
        if (sent <= 0) return false;
        offset += sent;
        left -= sent;
    }
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



#include "tcpserver.h"
#include "../kernel/tcpkernel.h"
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#endif

TCPServer::TCPServer() {
    m_SOCK = 0;
}

TCPServer::~TCPServer()
{
}

bool TCPServer::InitNetwork(char *ip, u_short port)
{
#ifdef _WIN32
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
#endif

    // 创建套接字
    m_SOCK = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SOCK == INVALID_SOCKET)
    {
        UninitNetwork((char*)"socket err");
        return false;
    }

    // 允许端口复用，服务端重启时不必等 TIME_WAIT
    int opt = 1;
    setsockopt(m_SOCK, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.s_addr = inet_addr(ip);
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
    std::thread td(TCPServer::ThreadAccept, this);
    td.detach();
    return true;
}

bool TCPServer::RecvData(SOCKET sockWaiter)
{
    // 1. 循环读满 4 字节包头：recv 可能只返回 1~3 字节，不能假设一次读全
    int len = 0;
    int offset = 0;
    int left = sizeof(int);
    while (left > 0)
    {
        int num = recv(sockWaiter, ((char*)&len) + offset, left, 0);
        if (num <= 0)
        {
            return false;
        }
        offset += num;
        left -= num;
    }

    // 2. 包长度合法性校验，避免垃圾长度触发 std::bad_alloc 导致进程崩溃
    if (len <= 0 || len > 1024 * 1024)
    {
        printf("invalid packet len: %d\n", len);
        return false;
    }

    // 3. 按长度读满整个包
    char* szbuf = new char[len];
    offset = 0;
    left = len;
    while(left > 0)
    {
        int num = recv(sockWaiter, szbuf + offset, left, 0);
        if (num > 0)
        {
            offset += num;
            left -= num;
        }
        else
        {
            delete[] szbuf;
            return false;
        }
    }

    // 4. 交给 kernel 处理
    TCPKernel::getKernel()->dealData(sockWaiter, szbuf);
    delete[] szbuf;
    return true;
}

bool TCPServer::SendData(SOCKET sockWaiter, char *buf, int bufLen)
{
    // 把 4 字节长度头和包体拼成一次 send：两次 write（小包 + 大包）
    // 会触发 Nagle 算法与延迟 ACK 互相等待，高延迟链路上吞吐会暴跌
    static thread_local std::vector<char> sendBuf;

    size_t total = sizeof(int) + (size_t)bufLen;
    if (sendBuf.size() < total)
        sendBuf.resize(total);

    memcpy(sendBuf.data(), &bufLen, sizeof(int));
    memcpy(sendBuf.data() + sizeof(int), buf, bufLen);

    int offset = 0;
    int left = (int)total;
    while (left > 0)
    {
        int sent = send(sockWaiter, sendBuf.data() + offset, left, 0);
        if (sent <= 0) return false;
        offset += sent;
        left -= sent;
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
#ifdef _WIN32
    WSACleanup();
#endif
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
    sockaddr_in clientAddr;
#ifdef _WIN32
    int size = sizeof(clientAddr);
#else
    socklen_t size = sizeof(clientAddr);
#endif
    while(1)
    {
        SOCKET sockwaiter = accept(pThis->m_SOCK, (sockaddr*)&clientAddr, &size);
        if (sockwaiter == INVALID_SOCKET)
            continue;

        // 关闭 Nagle：否则每个 4KB 分块都要等上一个包的 ACK，吞吐量被压到 4096/RTT
        int nodelay = 1;
        setsockopt(sockwaiter, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

        // 加大收发缓冲区，减少阻塞等待
        int bufSize = 256 * 1024;
        setsockopt(sockwaiter, SOL_SOCKET, SO_SNDBUF, (const char*)&bufSize, sizeof(bufSize));
        setsockopt(sockwaiter, SOL_SOCKET, SO_RCVBUF, (const char*)&bufSize, sizeof(bufSize));

        printf("Client %s : %d connect!!!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // 如果有连接——继续创建线程，用来和客户端通信
        std::thread td(TCPServer::ThreadProc, pThis, sockwaiter);
        td.detach();
    }
    closesocket(pThis->m_SOCK);      // 关闭套接字
    pThis->m_SOCK = 0;
#ifdef _WIN32
    WSACleanup();           // 卸载网络库
#endif
}

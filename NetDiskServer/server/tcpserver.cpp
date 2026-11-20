#include "tcpserver.h"
#include "../kernel/tcpkernel.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <exception>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
// 向已关闭的连接写入时不产生 SIGPIPE（否则会直接杀掉整个进程）
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#ifdef _WIN32
#define SEND_FLAGS 0
#else
#define SEND_FLAGS MSG_NOSIGNAL
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
    // 不用 thread_local / 不临时分配堆内存：避免 MinGW 下线程退出清理 TLS 时的堆损坏
    // Nagle 已关闭，分两次 send 不会产生延迟

    int offset = 0;
    int left = sizeof(bufLen);
    while (left > 0)
    {
        int sent = send(sockWaiter, ((char*)&bufLen) + offset, left, SEND_FLAGS);
        if (sent <= 0) return false;
        offset += sent;
        left -= sent;
    }

    offset = 0;
    left = bufLen;
    while (left > 0)
    {
        int sent = send(sockWaiter, buf + offset, left, SEND_FLAGS);
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
    // MySQL C API 要求：每个使用数据库的线程先做线程级初始化
    MysqlUtils::threadInit();

    while(1)
    {
        try
        {
            if(!pThis->RecvData(sockWaiter))    // 如果当前接收数据失败，则退出线程
                break;
        }
        catch (const std::exception& e)
        {
            // 没捕获的异常会让 std::terminate 直接结束整个进程
            printf("thread exception: %s\n", e.what());
            fflush(stdout);
            break;
        }
        catch (...)
        {
            printf("thread unknown exception\n");
            fflush(stdout);
            break;
        }
    }

    // 连接断开：清理在线表、关闭 socket（在锁内）并向好友推送下线状态
    TCPKernel::onSocketClosed(sockWaiter);

    MysqlUtils::threadEnd();
}

void TCPServer::ThreadAccept(TCPServer *pThis)
{
    while(1)
    {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int size = sizeof(clientAddr);
#else
        socklen_t size = sizeof(clientAddr);
#endif
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

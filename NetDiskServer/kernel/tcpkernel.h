#ifndef TCPKERNEL_H
#define TCPKERNEL_H

#include "IKernel.h"
#include "../server/tcpserver.h"
#include "../mysql/mysqlutils.h"
#include "../config.h"
#include <list>
#include <map>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifndef _WIN32
#include <ctime>
#include <sys/stat.h>
#endif

struct f_uploadInfo
{
    FILE* pf;           // 文件描述符
    long long fileID;   // 文件id
    long long userID;   // 用户id
    long long fileSize; // 文件大小
    long long pos;      // 传输了多少字节
};

class TCPKernel : public IKernel
{
private:
    TCPKernel();
    TCPKernel(TCPKernel&);
    TCPKernel& operator=(const TCPKernel&) = delete;

public:
    ~TCPKernel();
    // 启动服务器
    virtual bool open();
    // 关闭服务器
    virtual void close();
    // 处理数据
    virtual void dealData(SOCKET sock, char* buf);
    // 单例模式获取当前类地址
    // 懒汉模式——不支持线程安全
    // 饿汉模式
    static IKernel* getKernel()
    {
        // if(!m_Kernel)
        //     m_Kernel = new TCPKernel;
        return m_Kernel;
    }
    // 连接断开时由 tcpserver 调用：清理在线表并通知其好友下线
    static void onSocketClosed(SOCKET sock);

private:
    INet* m_TCPNet;
    MysqlUtils* m_SqlUtils;
    static IKernel* m_Kernel;
    std::list<f_uploadInfo*> m_lstuploadfileInfo;
    std::map<long long, SOCKET> m_mapIdToSock;      // userId -> socket
    std::map<SOCKET, long long> m_mapSockToId;      // socket -> userId
    std::mutex m_mutexOnline;                       // 保护上面两张表（多连接线程）
    std::mutex m_mutexUpload;                       // 保护 m_lstuploadfileInfo（多连接线程）

    bool isOnline(long long userId);
    void pushFriendStatus(long long userId, int online);
    // 在锁内给指定用户发送数据，避免与「对端刚好在关闭连接」发生竞争
    bool sendToUser(long long userId, char* buf, int len);
    void regRQ(SOCKET sock, char *buf);
    void loginRQ(SOCKET sock, char *buf);
    void getFileList(SOCKET sock, char *buf);
    void uploadFileInforq(SOCKET sock, char *buf);
    void uploadFilestreamrq(SOCKET sock, char *buf);
    void downloadfilerq(SOCKET sock, char *buf);
    void sharedfilerq(SOCKET sock, char *buf);
    void importfilerq(SOCKET sock, char *buf);
    void deletefilerq(SOCKET sock, char *buf);
    void addFriendRQ(SOCKET sock, char *buf);
    void getFriendListRQ(SOCKET sock, char *buf);
    void chatRQ(SOCKET sock, char *buf);

};

#endif // TCPKERNEL_H

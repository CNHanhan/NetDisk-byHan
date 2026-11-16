#ifndef TCPKERNEL_H
#define TCPKERNEL_H

#include "IKernel.h"
#include "../server/tcpserver.h"
#include "../mysql/mysqlutils.h"
#include <list>
#include <map>

#define FILEPATHBASE    "D:/disk/"

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

private:
    INet* m_TCPNet;
    MysqlUtils* m_SqlUtils;
    static IKernel* m_Kernel;
    std::list<f_uploadInfo*> m_lstuploadfileInfo;
    std::map<long long, SOCKET> m_mapIdToSock;
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

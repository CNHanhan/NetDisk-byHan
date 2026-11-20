#ifndef TCPKERNEL_H
#define TCPKERNEL_H

#include <QObject>
#include "IKernel.h"
#include "../Client/INet.h"
#include "../pack.h"

class TCPKernel : public QObject, public IKernel
{
    Q_OBJECT
public:
    explicit TCPKernel(QObject *parent = nullptr);
    ~TCPKernel();
    // 初始化网络
    bool connectServer(const char* ip = "127.0.0.1", u_short port = 8899);
    // 卸载网络
    void disconnectServer(const char* errmsg);
    // 接收数据
    void dealData(char* buf);
    // 发送数据
    bool sendData(char* buf, int bufLen);

signals:
    void signal_regrs(STRU_REG_RS*);
    void signal_loginrs(STRU_LOGIN_RS*);
    void signal_filelistrs(STRU_GETFILELIST_RS*);
    void signal_uploadfileinfors(STRU_UPLOADFILEINFO_RS*);
    void signal_downloadfilers(STRU_DOWNLOADFILE_RS*);
    void signal_sharedfilers(STRU_SHAREDFILE_RS*);
    void signal_importfilers(STRU_IMPORTFILE_RS*);
    void signal_addfriendrs(STRU_ADDFRIEND_RS*);
    void signal_friendlistrs(STRU_GETFRIENDLIST_RS*);
    void signal_chatrq(STRU_CHAT_RQ*);
    void signal_chatrs(STRU_CHAT_RS*);
    void signal_friendstatusrs(STRU_FRIENDSTATUS_RS*);


private:
    INet* m_pNet;
};

#endif // TCPKERNEL_H

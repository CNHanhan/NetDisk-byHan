#include "tcpkernel.h"
#include "../Client/tcpclient.h"


TCPKernel::TCPKernel(QObject *parent)
    : QObject{parent}
{
    m_pNet = new TCPClient(this);
}

TCPKernel::~TCPKernel()
{
    delete m_pNet;
}

bool TCPKernel::connectServer(const char *ip, u_short port)
{
    return m_pNet->InitNetwork(ip, port);
}

void TCPKernel::disconnectServer(const char *errmsg)
{
    m_pNet->UnInitNetwork(errmsg);
}

void TCPKernel::dealData(char* buf)
{
    // todo
    switch(*buf)
    {
    case PROTOCOL_REG_RS:           // 注册请求回复
        emit signal_regrs((STRU_REG_RS*)buf);
        break;
    case PROTOCOL_LOGIN_RS:           // 登录请求回复
        emit signal_loginrs((STRU_LOGIN_RS*)buf);
        break;
    case PROTOCOL_GETFILELIST_RS:           // 获取文件列表回复
        emit signal_filelistrs((STRU_GETFILELIST_RS*)buf);
        break;
    case PROTOCOL_UPLOADFILEINFO_RS:           // 上传文件信息回复
        emit signal_uploadfileinfors((STRU_UPLOADFILEINFO_RS*)buf);
        break;
    case PROTOCOL_DOWNLOADFILE_RS:           // 下载文件信息回复
        emit signal_downloadfilers((STRU_DOWNLOADFILE_RS*)buf);
        break;
    case PROTOCOL_SHAREDFILE_RS:           // 分享文件信息回复
        emit signal_sharedfilers((STRU_SHAREDFILE_RS*)buf);
        break;
    case PROTOCOL_IMPORTFILE_RS:           // 导入文件信息回复
        emit signal_importfilers((STRU_IMPORTFILE_RS*)buf);
        break;
    case PROTOCOL_ADDFRIEND_RS:
        emit signal_addfriendrs((STRU_ADDFRIEND_RS*)buf);
        break;
    case PROTOCOL_GETFRIENDLIST_RS:
        emit signal_friendlistrs((STRU_GETFRIENDLIST_RS*)buf);
        break;
    case PROTOCOL_CHAT_RQ:
        emit signal_chatrq((STRU_CHAT_RQ*)buf);
        break;
    case PROTOCOL_CHAT_RS:
        emit signal_chatrs((STRU_CHAT_RS*)buf);
        break;

    }
}

bool TCPKernel::sendData(char *buf, int bufLen)
{
    return m_pNet->SendData(buf,bufLen);
}

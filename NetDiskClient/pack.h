#ifndef PACK_H
#define PACK_H

#define MAXSIZE     45              // 最大字符串长度
#define SQLLEN      300

// 0-255
// 基础协议头
#define PROTOCOL_BASE       0

// 注册
#define PROTOCOL_REG_RQ     (PROTOCOL_BASE + 1) // 注册请求
#define PROTOCOL_REG_RS     (PROTOCOL_BASE + 2) // 注册回复
// 登录
#define PROTOCOL_LOGIN_RQ     (PROTOCOL_BASE + 3) // 登录请求
#define PROTOCOL_LOGIN_RS     (PROTOCOL_BASE + 4) // 登录回复

// 获取文件列表
#define PROTOCOL_GETFILELIST_RQ     (PROTOCOL_BASE + 5)     // 文件列表请求
#define PROTOCOL_GETFILELIST_RS     (PROTOCOL_BASE + 6)     // 文件列表回复

// 上传文件——正常传输，秒传，断点续传
#define PROTOCOL_UPLOADFILEINFO_RQ     (PROTOCOL_BASE + 7)     // 上传文件信息请求
#define PROTOCOL_UPLOADFILEINFO_RS     (PROTOCOL_BASE + 8)     // 上传文件信息回复

#define PROTOCOL_UPLOADFILEBLOCK_RQ     (PROTOCOL_BASE + 9)     // 上传文件流


// 下载文件
#define PROTOCOL_DOWNLOADFILE_RQ     (PROTOCOL_BASE + 10)     // 下载文件请求
#define PROTOCOL_DOWNLOADFILE_RS     (PROTOCOL_BASE + 11)     // 下载文件回复


// 分享文件
#define PROTOCOL_SHAREDFILE_RQ          (PROTOCOL_BASE + 12)    // 分享文件请求包
#define PROTOCOL_SHAREDFILE_RS          (PROTOCOL_BASE + 13)    // 分享文件回复包

// 提取文件
#define PROTOCOL_IMPORTFILE_RQ          (PROTOCOL_BASE + 14)    // 提取文件请求包
#define PROTOCOL_IMPORTFILE_RS          (PROTOCOL_BASE + 15)    // 提取文件回复包

// 心跳包
#define PROTOCOL_PING_RS          (PROTOCOL_BASE + 17)

// 协议包头
struct STUR_BASE
{
    char m_type;
};


// 心跳数据包
struct STRU_PING_RS : public STUR_BASE
{
    STRU_PING_RS()
    {
        m_type = PROTOCOL_PING_RS;
    }
    char m_ret;
};

// 添加好友


// 好友聊天



// 申请账号数据包
struct STRU_REG_RQ : public STUR_BASE
{
    STRU_REG_RQ()
    {
        m_type = PROTOCOL_REG_RQ;
    }
    // 账号
    char m_user[MAXSIZE];
    // 密码
    char m_pwd[MAXSIZE];
    // 手机号
    long long m_tel;
};

#define REG_ERR     0
#define REG_SUC     1

// 申请账号回复包
struct STRU_REG_RS : public STUR_BASE
{
    STRU_REG_RS()
    {
        m_type = PROTOCOL_REG_RS;
    }
    // 回复结果，注册成功或失败
    char m_ret;
};



// 登录请求包
struct STRU_LOGIN_RQ : public STUR_BASE
{
    STRU_LOGIN_RQ()
    {
        m_type = PROTOCOL_LOGIN_RQ;
    }
    // 账号
    char m_user[MAXSIZE];
    // 密码
    char m_pwd[MAXSIZE];
};

#define LOGIN_SUC       0
#define LOGIN_PWD_ERR   1
#define LOGIN_USER_ERR  2
// 登录回复包
struct STRU_LOGIN_RS : public STUR_BASE
{
    STRU_LOGIN_RS()
    {
        m_type = PROTOCOL_LOGIN_RS;
    }
    // 登录成功或失败
    char m_ret;
    // 用户id
    long long m_userId;
};


// 获取文件列表请求包
struct STRU_GETFILELIST_RQ : public STUR_BASE
{
    STRU_GETFILELIST_RQ()
    {
        m_type = PROTOCOL_GETFILELIST_RQ;
    }
    long long m_userId;
};

// 返回文件名字，文件大小，文件上传时间
struct FILEINFO
{
    long long m_fileID;
    int m_finish;
    char m_filename[MAXSIZE];
    char m_uploadtime[MAXSIZE];
    long long m_filesize;
};

#define FILENUM     1
// 文件列表回复包
struct STRU_GETFILELIST_RS : public STUR_BASE
{
    STRU_GETFILELIST_RS()
    {
        m_type = PROTOCOL_GETFILELIST_RS;
    }
    FILEINFO m_fileInfo[FILENUM];
    int m_fileNum;
};




// 上传文件信息请求包
struct STRU_UPLOADFILEINFO_RQ : public STUR_BASE
{
    STRU_UPLOADFILEINFO_RQ()
    {
        m_type = PROTOCOL_UPLOADFILEINFO_RQ;
    }
    FILEINFO m_fileinfo;
    char m_md5[MAXSIZE];
    long long m_userId;
};


#define UPLOAD_FILEINFO_NORMAL      1       // 正常传
#define UPLOAD_FILEINFO_CONTINUE    2       // 断点续传
#define UPLOAD_FILEINFO_FALSH       4       // 秒传
#define UPLOAD_FILEINFO_REPEAT      8       // 重复上传


// 上传文件信息回复包   正常传，断点续传，秒传，重复上传
struct STRU_UPLOADFILEINFO_RS : public STUR_BASE
{
    STRU_UPLOADFILEINFO_RS()
    {
        m_type = PROTOCOL_UPLOADFILEINFO_RS;
    }
    char m_ret;
    char m_md5[MAXSIZE];        // 文件唯一标识符
    long long m_pos;            // 断点续传的位置
    long long m_fileId;         // 文件在数据库中的唯一id
};

#define ONEPAGE         4096
// 上传文件流数据包
struct STRU_UPLOADFILESTREAM_RQ : public STUR_BASE
{
    STRU_UPLOADFILESTREAM_RQ()
    {
        m_type = PROTOCOL_UPLOADFILEBLOCK_RQ;
    }
    long long m_userID;
    long long m_fileID;
    char m_Stream[ONEPAGE];
    int m_StreamLen;
};





// 下载文件请求包
struct STRU_DOWNLOADFILE_RQ : public STUR_BASE
{
    STRU_DOWNLOADFILE_RQ()
    {
        m_type = PROTOCOL_DOWNLOADFILE_RQ;
    }
    long long userId;
    long long fileId;
};

#define DOWNLOADFILE_SUC            0
#define DOWNLOADFILE_ERR            1

// 下载文件回复包----包含文件数据
struct STRU_DOWNLOADFILE_RS : public STUR_BASE
{
    STRU_DOWNLOADFILE_RS()
    {
        m_type = PROTOCOL_DOWNLOADFILE_RS;
    }
    char m_ret;     // 是否可以下载
    char fileStream[ONEPAGE];
    int fileStreamLen;
    long long fileSize;
    long long fileId;
};


// 分享文件请求包
struct STRU_SHAREDFILE_RQ : public STUR_BASE
{
    STRU_SHAREDFILE_RQ()
    {
        m_type = PROTOCOL_SHAREDFILE_RQ;
    }
    long long userId;
    long long fileId;
    char sharedtime[MAXSIZE];
};


#define SHARED_SUC      0
#define SHARED_ERR      1
#define CODELEN         9
// 分享文件回复包
struct STRU_SHAREDFILE_RS : public STUR_BASE
{
    STRU_SHAREDFILE_RS()
    {
        m_type = PROTOCOL_SHAREDFILE_RS;
    }
    char m_ret;
    char m_code[CODELEN];
};






// 提取文件请求包
struct STRU_IMPORTFILE_RQ : public STUR_BASE
{
    STRU_IMPORTFILE_RQ()
    {
        m_type = PROTOCOL_IMPORTFILE_RQ;
    }
    long long userId;
    char code[CODELEN];
};
// 服务器收到请求，验证提取码是否有效，当前用户是否已有这个文件
// 成功-》映射表中添加当前用户与该文件的映射->查询文件信息，填充回复包

#define IMPORT_SUC      0
#define IMPORT_ERR      1
// 提取文件回复包
struct STRU_IMPORTFILE_RS : public STUR_BASE
{
    STRU_IMPORTFILE_RS()
    {
        m_type = PROTOCOL_IMPORTFILE_RS;
    }
    char m_ret;
    FILEINFO info;
};



// 删除文件
#define PROTOCOL_DELETEFILE_RQ          (PROTOCOL_BASE + 16)    // 删除文件请求包

// 删除文件数据包
struct STRU_DELETEFILE_RQ : public STUR_BASE
{
    STRU_DELETEFILE_RQ()
    {
        m_type = PROTOCOL_DELETEFILE_RQ;
    }
    long long userId;       // 哪个用户
    long long fileId;       // 要删除哪个文件
};


// 客户端选中某行，点击删除文件，客户端检查是否选中文件
// 选中文件——提取文件的id，和用户id构成发送包——删除表格中的文件显示，删除vector中的文件id——发送到服务器        / 错误提示



// 服务器接收到数据包
// 检查用户是否拥有该文件
// 没有——返回
// 更新数据库，删除当前用户和这个文件的映射
// 更新数据，file表使文件引用计数-1
// 检查文件引用计数是否小于等于0,文件路径，文件完整状态
// 遍历服务器存储上传文件信息的链表，找到当前文件，然后将文件关闭，并且将文件信息从链表中移除
// 更新当前file表，删除这个文件所在行
// 是——服务器真实要删除这个文件，DeleteFile()

// ==================== 好友与聊天协议 ====================
#define PROTOCOL_ADDFRIEND_RQ       (PROTOCOL_BASE + 18) // 添加好友请求
#define PROTOCOL_ADDFRIEND_RS       (PROTOCOL_BASE + 19) // 添加好友回复
#define PROTOCOL_GETFRIENDLIST_RQ   (PROTOCOL_BASE + 20) // 获取好友列表请求
#define PROTOCOL_GETFRIENDLIST_RS   (PROTOCOL_BASE + 21) // 获取好友列表回复
#define PROTOCOL_CHAT_RQ            (PROTOCOL_BASE + 22) // 聊天消息请求（服务端直接转发）
#define PROTOCOL_CHAT_RS            (PROTOCOL_BASE + 23) // 聊天消息回复（对方不在线提示）

// 1. 添加好友请求包
struct STRU_ADDFRIEND_RQ : public STUR_BASE
{
    STRU_ADDFRIEND_RQ() { m_type = PROTOCOL_ADDFRIEND_RQ; }
    long long m_userId;            // 自己的用户ID
    char m_friendName[MAXSIZE];    // 要添加的好友用户名
};

#define ADDFRIEND_SUC       0   // 添加成功
#define ADDFRIEND_NO_USER   1   // 用户不存在
#define ADDFRIEND_EXIST     2   // 已经是好友或不能加自己
// 添加好友回复包
struct STRU_ADDFRIEND_RS : public STUR_BASE
{
    STRU_ADDFRIEND_RS() { m_type = PROTOCOL_ADDFRIEND_RS; }
    char m_ret;
    long long m_friendId;
    char m_friendName[MAXSIZE];
};

// 2. 获取好友列表请求包
struct STRU_GETFRIENDLIST_RQ : public STUR_BASE
{
    STRU_GETFRIENDLIST_RQ() { m_type = PROTOCOL_GETFRIENDLIST_RQ; }
    long long m_userId;
};

struct FRIENDINFO
{
    long long m_friendId;
    char m_friendName[MAXSIZE];
};

// 获取好友列表回复包（每次发1个好友信息，和文件列表机制一致）
struct STRU_GETFRIENDLIST_RS : public STUR_BASE
{
    STRU_GETFRIENDLIST_RS() { m_type = PROTOCOL_GETFRIENDLIST_RS; }
    FRIENDINFO m_friendInfo[FILENUM];
    int m_friendNum;
};

// 3. 聊天请求/转发包
#define CHATLEN 512
struct STRU_CHAT_RQ : public STUR_BASE
{
    STRU_CHAT_RQ() { m_type = PROTOCOL_CHAT_RQ; }
    long long m_sendUserId;
    char m_sendUserName[MAXSIZE];
    long long m_recvUserId;
    char m_content[CHATLEN];
};

#define CHAT_SUC        0
#define CHAT_OFFLINE    1
struct STRU_CHAT_RS : public STUR_BASE
{
    STRU_CHAT_RS() { m_type = PROTOCOL_CHAT_RS; }
    char m_ret;
    long long m_recvUserId;
};










#endif // PACK_H

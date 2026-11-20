#include "tcpkernel.h"
#include "../pack.h"

IKernel* TCPKernel::m_Kernel = new TCPKernel;

TCPKernel::TCPKernel()
{
    m_TCPNet = new TCPServer;
    m_SqlUtils = new MysqlUtils;
}

TCPKernel::~TCPKernel()
{
    delete m_TCPNet;
    delete m_SqlUtils;
}

bool TCPKernel::open()
{
    if(!m_TCPNet->InitNetwork((char*)SERVER_LISTEN_IP, SERVER_LISTEN_PORT))
    {
        printf("[open] init network failed\n");
        return false;
    }
    if(!m_SqlUtils->ConnectMysql(MYSQL_HOST, MYSQL_USER, MYSQL_PASSWD, MYSQL_DB))
    {
        printf("[open] connect mysql failed: user=%s host=%s port=%d db=%s\n",
               MYSQL_USER, MYSQL_HOST, MYSQL_PORT, MYSQL_DB);
        return false;
    }
    return true;
}

void TCPKernel::close()
{
    m_TCPNet->UninitNetwork((char*)"server is close");
    m_SqlUtils->DisConnect();
}

void TCPKernel::dealData(SOCKET sock, char *buf, int len)
{
    // 校验包体长度是否与本端结构体一致。
    // 两端 pack.h 不同步（比如用了 long / 加了字段）时，这里会立刻打印出来，
    // 而不是“静默错位”导致功能看起来卡死。
#define CHECK_PACK_LEN(T) \
    if (len != (int)sizeof(T)) { \
        printf("packet len mismatch, type=%d, got=%d, expect=%d\n", \
               (int)(unsigned char)*buf, len, (int)sizeof(T)); \
        fflush(stdout); \
        return; \
    }

    switch (*buf) {
    case PROTOCOL_REG_RQ:   // 注册请求
        CHECK_PACK_LEN(STRU_REG_RQ);
        regRQ(sock, buf);
        break;
    case PROTOCOL_LOGIN_RQ:   // 登录请求
        CHECK_PACK_LEN(STRU_LOGIN_RQ);
        loginRQ(sock, buf);
        break;
    case PROTOCOL_GETFILELIST_RQ:   // 获取文件列表请求
        CHECK_PACK_LEN(STRU_GETFILELIST_RQ);
        getFileList(sock, buf);
        break;
    case PROTOCOL_UPLOADFILEINFO_RQ:   // 上传文件信息请求
        CHECK_PACK_LEN(STRU_UPLOADFILEINFO_RQ);
        uploadFileInforq(sock, buf);
        break;
    case PROTOCOL_UPLOADFILEBLOCK_RQ:   // 上传文件流
        CHECK_PACK_LEN(STRU_UPLOADFILESTREAM_RQ);
        uploadFilestreamrq(sock, buf);
        break;
    case PROTOCOL_DOWNLOADFILE_RQ:   // 下载文件请求
        CHECK_PACK_LEN(STRU_DOWNLOADFILE_RQ);
        downloadfilerq(sock, buf);
        break;
    case PROTOCOL_SHAREDFILE_RQ:   // 分享文件请求
        CHECK_PACK_LEN(STRU_SHAREDFILE_RQ);
        sharedfilerq(sock, buf);
        break;
    case PROTOCOL_IMPORTFILE_RQ:   // 提取文件请求
        CHECK_PACK_LEN(STRU_IMPORTFILE_RQ);
        importfilerq(sock, buf);
        break;
    case PROTOCOL_DELETEFILE_RQ:   // 删除文件请求
        CHECK_PACK_LEN(STRU_DELETEFILE_RQ);
        deletefilerq(sock, buf);
        break;
    case PROTOCOL_ADDFRIEND_RQ:
        CHECK_PACK_LEN(STRU_ADDFRIEND_RQ);
        addFriendRQ(sock, buf);
        break;
    case PROTOCOL_GETFRIENDLIST_RQ:
        CHECK_PACK_LEN(STRU_GETFRIENDLIST_RQ);
        getFriendListRQ(sock, buf);
        break;
    case PROTOCOL_CHAT_RQ:
        CHECK_PACK_LEN(STRU_CHAT_RQ);
        chatRQ(sock, buf);
        break;
    }

#undef CHECK_PACK_LEN
}

void TCPKernel::regRQ(SOCKET sock, char *buf)
{
    STRU_REG_RQ* psrr = (STRU_REG_RQ*)buf;
    STRU_REG_RS srr;
    srr.m_ret = REG_ERR;

    // 参数校验（客户端也会校验，这里防绕过客户端直接发包）
    if (psrr->m_user[0] == '\0' || strlen(psrr->m_pwd) < 4 || psrr->m_tel <= 0)
    {
        m_TCPNet->SendData(sock, (char*)&srr, sizeof(srr));
        return;
    }
    char filePath[MAX_PATH] = {0};
    // 调用数据库模块，将信息构造为sql语句，执行
    char sql[SQLLEN] = {0};
    sprintf(sql, "insert into user(u_name, u_password, u_tel) values('%s', '%s', %lld)", psrr->m_user, psrr->m_pwd, psrr->m_tel);
    if(m_SqlUtils->Update(sql))
    {
        // 数据插入成功
        srr.m_ret = REG_SUC;
        // 为用户创建自己的目录，存放他的文件，根据用户id创建目录
        std::list<std::string> lst;
        sprintf(sql, "select u_id from user where u_tel = %lld", psrr->m_tel);
        m_SqlUtils->Select(sql, 1, lst);
        if(!lst.empty())
        {
            std::string id = lst.front();
            lst.pop_front();
            // 为用户创建目录
            sprintf(filePath, "%s%s", FILEPATHBASE, id.c_str());
#ifdef _WIN32
            CreateDirectoryA(filePath, 0);
#else
            mkdir(filePath, 0777);
#endif
        }
    }
    // 回复
    m_TCPNet->SendData(sock, (char*)&srr, sizeof(srr));
}

void TCPKernel::loginRQ(SOCKET sock, char *buf)
{
    // 将buf转为登录请求结构体
    STRU_LOGIN_RQ* pslr = (STRU_LOGIN_RQ*)buf;
    printf("login request: user=%s\n", pslr->m_user);
    fflush(stdout);

    // 参数校验：空用户名/空密码直接拒绝
    if (pslr->m_user[0] == '\0' || pslr->m_pwd[0] == '\0')
    {
        STRU_LOGIN_RS rs;
        rs.m_ret = LOGIN_PWD_ERR;
        rs.m_userId = 0;
        m_TCPNet->SendData(sock, (char*)&rs, sizeof(rs));
        return;
    }
    char sql[SQLLEN] = {0};
    sprintf(sql, "select u_id, u_password from user where u_name = '%s'", pslr->m_user);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 2, lst);
    STRU_LOGIN_RS slr;
    slr.m_ret = LOGIN_USER_ERR;
    if(!lst.empty())
    {
        slr.m_ret = LOGIN_PWD_ERR;
        // 可以获取密码，用户存在
        std::string id = lst.front();
        lst.pop_front();
        std::string pwd = lst.front();
        lst.pop_front();
        if(strcmp(pslr->m_pwd, pwd.c_str()) == 0)
        {
            // 密码正确
            slr.m_ret = LOGIN_SUC;
            slr.m_userId = atoll(id.c_str());
            {
                std::lock_guard<std::mutex> lk(m_mutexOnline);
                m_mapIdToSock[slr.m_userId] = sock;   // userId -> socket
                m_mapSockToId[sock] = slr.m_userId;   // socket -> userId
            }
        }
    }
    // 用户不存在
    m_TCPNet->SendData(sock, (char*)&slr, sizeof(slr));

    // 登录成功后通知在线好友：我上线了
    if(slr.m_ret == LOGIN_SUC)
        pushFriendStatus(slr.m_userId, 1);
}

void TCPKernel::getFileList(SOCKET sock, char *buf)
{
    // 查询当前用户拥有的文件列表
    // 将buf强转为STRU_GETFILELIST_RQ类型
    STRU_GETFILELIST_RQ* psgr = (STRU_GETFILELIST_RQ*) buf;
    // 根据用户id生成对应的sql语句
    char sql[SQLLEN] = {0};
    sprintf(sql, "select f_name, f_size, f_uploadtime, f_id, f_finish from myview where u_id = %lld", psgr->m_userId);
    // 信息存储在lst
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 5, lst);
    // 声明回复包
    STRU_GETFILELIST_RS sgr;
    // 解析返回的信息---循环
    int i = 0;
    while (!lst.empty())
    {
        // 获取文件名字，大小，上传时间，构造为fileinfo结构体
        std::string fileName = lst.front();
        lst.pop_front();
        std::string fileSize = lst.front();
        lst.pop_front();
        std::string uploadTime = lst.front();
        lst.pop_front();
        std::string fileID = lst.front();
        lst.pop_front();
        std::string finish = lst.front();
        lst.pop_front();
        // 文件名最长可能是 varchar(255)，目标只有 45 字节，必须截断
        strncpy(sgr.m_fileInfo[i].m_filename, fileName.c_str(), MAXSIZE - 1);
        sgr.m_fileInfo[i].m_filename[MAXSIZE - 1] = '\0';
        strncpy(sgr.m_fileInfo[i].m_uploadtime, uploadTime.c_str(), MAXSIZE - 1);
        sgr.m_fileInfo[i].m_uploadtime[MAXSIZE - 1] = '\0';
        sgr.m_fileInfo[i].m_filesize = atoll(fileSize.c_str());
        sgr.m_fileInfo[i].m_fileID = atoll(fileID.c_str());
        sgr.m_fileInfo[i].m_finish = atoi(finish.c_str());
        // 记录当前文件数量
        i++;
        // 如果当前fileinfo结构体数量达到宏定义的限制 或者 当前查询返回的链表为空
        if (i == FILENUM || lst.empty())
        {
            // 发送回复
            sgr.m_fileNum = i;
            m_TCPNet->SendData(sock, (char*)&sgr, sizeof(sgr));
            i = 0;
            memset(sgr.m_fileInfo, 0, sizeof(sgr.m_fileInfo));
        }
    }
}

void TCPKernel::uploadFileInforq(SOCKET sock, char *buf)
{
    // 分析当前文件是正常穿，秒传，断点续传，重复上传
    // 1 检查文件是否存在在数据库——》MD5
        // 存在
            // 自己上传的
                // 断点续传
                // 重复上传
            // 别人传的——》秒传
        // 不存在-》正常传
    // 发送回复
    STRU_UPLOADFILEINFO_RS sur;

    // 上传列表是多连接线程共享的，必须加锁
    std::lock_guard<std::mutex> lkUpload(m_mutexUpload);

    STRU_UPLOADFILEINFO_RQ* psur = (STRU_UPLOADFILEINFO_RQ*)buf;
    strcpy(sur.m_md5, psur->m_md5);
    sur.m_pos = 0;
    char sql[1024] = {0};   // 拼接文件路径，比 SQLLEN(300) 更大的缓冲，避免被截断
    sprintf(sql,"select f_id from file where f_md5 = '%s'", psur->m_md5);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);
    if(!lst.empty())
    {
        // 存在文件
        // 使用文件id去映射表查询有哪些用户拥有文件
        long long fileId = atoll(lst.front().c_str());
		sur.m_fileId = fileId;
        sprintf(sql,"select u_id from user_file where f_id = %lld", fileId);
        lst.pop_front();
        m_SqlUtils->Select(sql, 1, lst);
        auto it = lst.begin();
        while(it != lst.end())
        {
            if(atoll((*it).c_str()) == psur->m_userId)
            {
                // 自己上传
                // 先认定是重复上传
                sur.m_ret = UPLOAD_FILEINFO_REPEAT;
                // 遍历m_lstuploadfileInfo 文件id如过和传入的文件id样
                auto ite = m_lstuploadfileInfo.begin();
                while(ite != m_lstuploadfileInfo.end())
                {
                    if(fileId == (*ite)->fileID)
                    {
                        // 断点续传
                        sur.m_ret = UPLOAD_FILEINFO_CONTINUE;
                        sur.m_pos = (*ite)->pos;
                        strcpy(sur.m_md5, psur->m_md5);
                        sur.m_fileId = fileId;
                        break;
                    }
                    ite++;
                }
                m_TCPNet->SendData(sock, (char*)&sur, sizeof(sur));
                return;
            }
            it++;
        }
        //别人上传
        sur.m_ret = UPLOAD_FILEINFO_FALSH;
        // 遍历m_lstuploadfileInfo 文件ID如过和传入的文件OD一样
        auto ite = m_lstuploadfileInfo.begin();
        while(ite != m_lstuploadfileInfo.end())
        {
            if(fileId == (*ite)->fileID)
            {
                // 断点续传
                sur.m_ret = UPLOAD_FILEINFO_CONTINUE | UPLOAD_FILEINFO_FALSH;
                sur.m_pos = (*ite)->pos;
                strcpy(sur.m_md5, psur->m_md5);
                break;
            }
            ite++;
        }
        // 更新映射表
        sprintf(sql,"insert into user_file(u_id, f_id) values(%lld, %lld)", psur->m_userId, fileId);
        m_SqlUtils->Update(sql);
        // 文件表的当前文件引用计数+1
        sprintf(sql,"update file set f_count = f_count + 1 where f_id = %lld",fileId);
        m_SqlUtils->Update(sql);
        m_TCPNet->SendData(sock, (char*)&sur, sizeof(sur));

    }
    else {
        // 不存在该文件-》将文件信息写入到数据库-》将文件与用户映射
        // 匹配文件路径
        char filePath[MAX_PATH] = {0};
        sprintf(filePath, "%s%lld/%s",FILEPATHBASE, psur->m_userId, psur->m_md5);

        sprintf(sql,"insert into file(f_name, f_size, f_uploadtime, f_path, f_md5) values('%s', %lld, '%s', '%s', '%s')",
                psur->m_fileinfo.m_filename, psur->m_fileinfo.m_filesize,psur->m_fileinfo.m_uploadtime,
                filePath, psur->m_md5);
        if(m_SqlUtils->Update(sql))
        {
            // 查询文件id
            sprintf(sql,"select f_id from file where f_md5 = '%s'", psur->m_md5);
            m_SqlUtils->Select(sql, 1, lst);
            if(lst.empty())
            {
                printf("insert file ok but select f_id failed\n");
                return;
            }
            long long fileId = atoll(lst.front().c_str());
            // 将用户与文件id添加到映射表
            sprintf(sql, "insert into user_file (u_id, f_id) values(%lld, %lld)", psur->m_userId, fileId);
            m_SqlUtils->Update(sql);
            sur.m_fileId = fileId;
            sur.m_ret = UPLOAD_FILEINFO_NORMAL;
            m_TCPNet->SendData(sock,(char*)&sur,sizeof(sur));
            // 提前创建文件
            f_uploadInfo* p = new f_uploadInfo;
            FILE* pf = fopen(filePath, "wb");
            p->pf = pf;
            p->fileID = fileId;
            p->pos = 0;
            p->fileSize = psur->m_fileinfo.m_filesize;
            p->userID = psur->m_userId;
            m_lstuploadfileInfo.push_back(p);
        }
    }
}

void TCPKernel::uploadFilestreamrq(SOCKET sock, char *buf)
{
    std::lock_guard<std::mutex> lkUpload(m_mutexUpload);

    STRU_UPLOADFILESTREAM_RQ* psur = (STRU_UPLOADFILESTREAM_RQ*)buf;
    // 找到要写入的文件
    auto it = m_lstuploadfileInfo.begin();
    f_uploadInfo* p = nullptr;
    while(it != m_lstuploadfileInfo.end())
    {
        if((*it)->fileID == psur->m_fileID)
        {
            p = *it;
            break;
        }
        it++;
    }
    if(!p)  return;

    // 校验包内长度，防止异常/恶意包造成越界读
    if(psur->m_StreamLen <= 0 || psur->m_StreamLen > ONEPAGE)
        return;

    // 写入
    int num = fwrite(psur->m_Stream, 1, psur->m_StreamLen, p->pf);
    if(num < 0) num = 0;
    p->pos += num;
    if(p->pos >= p->fileSize)
    {
        // 文件传输完成
        char sql[SQLLEN] = {0};
        sprintf(sql,"update file set f_finish = 1 where f_id = %lld",p->fileID);
        m_SqlUtils->Update(sql);
        fclose(p->pf);
        delete p;
        m_lstuploadfileInfo.erase(it);
    }
}

void TCPKernel::downloadfilerq(SOCKET sock, char *buf)
{
    char sql[SQLLEN] = {0};
    STRU_DOWNLOADFILE_RQ* psdr = (STRU_DOWNLOADFILE_RQ*)buf;
    sprintf(sql,"select f_finish, f_path, f_size from myview where u_id = %lld and f_id = %lld", psdr->userId, psdr->fileId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 3, lst);
    STRU_DOWNLOADFILE_RS sdr;
    sdr.m_ret = DOWNLOADFILE_ERR;
    if(!lst.empty() && atoi(lst.front().c_str()))
    {
        // 存在数据，并且文件完整，可以下载
        sdr.m_ret = DOWNLOADFILE_SUC;
        lst.pop_front();
        std::string filePath = lst.front();
        lst.pop_front();
        long long fileSize = atoll(lst.front().c_str());
        lst.pop_front();
        // 发送文件回复
        sdr.fileId = psdr->fileId;
        sdr.fileSize = fileSize;
        FILE* pf = fopen(filePath.c_str(),"rb");
        if(!pf)
        {
            // 数据库里有记录但磁盘文件不存在（比如被手动删了），不能直接 fread(NULL)
            printf("open file failed: %s\n", filePath.c_str());
            sdr.m_ret = DOWNLOADFILE_ERR;
            m_TCPNet->SendData(sock, (char*)&sdr, sizeof(sdr));
            return;
        }
        while(1)
        {
            // 发送数据
            sdr.fileStreamLen = fread(sdr.fileStream,1,ONEPAGE,pf);
            if(sdr.fileStreamLen <= 0)
            {
                fclose(pf);
                break;
            }
            // 客户端中途断开时立即停止，别继续往死 socket 写
            if(!m_TCPNet->SendData(sock, (char*)&sdr, sizeof(sdr)))
            {
                printf("send file block failed, abort download\n");
                fclose(pf);
                break;
            }
        }
        return;
    }
    m_TCPNet->SendData(sock, (char*)&sdr, sizeof(sdr));

}

void TCPKernel::sharedfilerq(SOCKET sock, char *buf)
{
    STRU_SHAREDFILE_RQ* pssr = (STRU_SHAREDFILE_RQ*)buf;
    char sql[SQLLEN] = {0};
    sprintf(sql,"select f_finish from myview where u_id = %lld and f_id = %lld",pssr->userId, pssr->fileId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);
    STRU_SHAREDFILE_RS ssr;
    ssr.m_ret = SHARED_ERR;
    if(!lst.empty() && atoi(lst.front().c_str()))
    {
        ssr.m_ret = SHARED_SUC;
        srand(time(0));
        for(int i = 0; i < CODELEN - 1; i++)
        {
            // 写入随机数
            int n = rand() % 36;
            if(n < 10)
                ssr.m_code[i] = n + '0';
            else
                ssr.m_code[i] = n - 10 + 'A';
        }
        ssr.m_code[CODELEN - 1] = 0;
        // 添加分享信息到分享文件表
        sprintf(sql,"insert into shared_file(u_id,f_id,code,sharedtime) values(%lld,%lld,'%s','%s')",
                pssr->userId,pssr->fileId,ssr.m_code,pssr->sharedtime);
        m_SqlUtils->Update(sql);
    }

    m_TCPNet->SendData(sock, (char*)&ssr,sizeof(ssr));

}

void TCPKernel::importfilerq(SOCKET sock, char *buf)
{
    // 查询数据库，查询分享文件表，根据分享码拿到文件id，
    // select f_id from shared_file where code = 'M7LQCT5V' and sharedtime > now()
    // 查询数据库，根据文件id查找当前的用户是否已经拥有文件
    // 查询数据库,查询视图-》文件完整性，文件名，文件大小，上传时间，文件id查询出来
    // 如果都没有问题，文件引用计数+1，当前用户和文件进行映射。则构建回复包将文件信息发送会客户端
    STRU_IMPORTFILE_RQ* psir = (STRU_IMPORTFILE_RQ*)buf;
    char sql[SQLLEN] = {0};
    sprintf(sql, "select f_id from shared_file where code = '%s' and sharedtime > now()", psir->code);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);
    STRU_IMPORTFILE_RS sir;
    sir.m_ret = IMPORT_ERR;
    if(!lst.empty())
    {
        // 有文件id
        long long fileId = atoll(lst.front().c_str());
        lst.pop_front();
        sprintf(sql,"insert into user_file(u_id, f_id) values(%lld, %lld)",psir->userId, fileId);
        if(m_SqlUtils->Update(sql))
        {
            sprintf(sql,"select f_finish, f_name, f_size, f_uploadtime from file where f_id = %lld", fileId);
            m_SqlUtils->Select(sql, 4, lst);
            if(!lst.empty() && atoi(lst.front().c_str()))
            {
                // 文件完整
                lst.pop_front();
                std::string filename = lst.front();
                lst.pop_front();
                long long  filesize = atoll(lst.front().c_str());
                lst.pop_front();
                std::string fileuploadtime = lst.front();
                lst.pop_front();
                // 信息填充，将用户和文件映射，文件引用计数+1
                sprintf(sql,"update file set f_count = f_count + 1 where f_id = %lld", fileId);
                m_SqlUtils->Update(sql);

                sir.m_ret = IMPORT_SUC;
                strncpy(sir.info.m_filename, filename.c_str(), MAXSIZE - 1);
                sir.info.m_filename[MAXSIZE - 1] = '\0';
                strncpy(sir.info.m_uploadtime, fileuploadtime.c_str(), MAXSIZE - 1);
                sir.info.m_uploadtime[MAXSIZE - 1] = '\0';
                sir.info.m_finish = 1;
                sir.info.m_fileID = fileId;
                sir.info.m_filesize = filesize;
            }
        }
    }
    m_TCPNet->SendData(sock,(char*)&sir,sizeof(sir));
}

void TCPKernel::deletefilerq(SOCKET sock, char *buf)
{
    std::lock_guard<std::mutex> lkUpload(m_mutexUpload);

    STRU_DELETEFILE_RQ* psdr = (STRU_DELETEFILE_RQ*)buf;
    // 查询用户是否拥有该文件
    char sql[SQLLEN] = {0};
    // 在视图中一次取出文件完整行，引用计数，文件路径
    sprintf(sql,"select f_finish, f_count, f_path from myview where u_id = %lld and f_id = %lld",psdr->userId, psdr->fileId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql,3,lst);
    // 如果存在这条记录
    if(!lst.empty())
    {
        // 检查文件完整性
        if(atoi(lst.front().c_str()) == 0)
        {
            // 文件不完整，删除服务器链表中的相关信息
            auto it = m_lstuploadfileInfo.begin();
            while(it != m_lstuploadfileInfo.end())
            {
                if((*it)->fileID == psdr->fileId)
                {
                    fclose((*it)->pf);      // 关闭文件描述符
                    delete *it;             // 释放存储信息的结构体内存
                    m_lstuploadfileInfo.erase(it);      // 删除该节点
                    break;
                }
                it++;
            }
        }
        lst.pop_front();

        // 删除用户与文件映射
        sprintf(sql,"delete from user_file where u_id = %lld and f_id = %lld",psdr->userId, psdr->fileId);
        m_SqlUtils->Update(sql);

        long long f_count = atoll(lst.front().c_str()); // 取出文件引用计数
        lst.pop_front();
        // 根据文件引用计数更新file表
        if(f_count > 1)
        {
            // 使文件引用计数-1
            sprintf(sql,"update file set f_count = f_count - 1 where f_id = %lld", psdr->fileId);
            m_SqlUtils->Update(sql);
        }
        else
        {
            // 文件引用计数小于等于1，删除该行文件记录，并且删除该文件
            sprintf(sql, "delete from file where f_id = %lld", psdr->fileId);
            m_SqlUtils->Update(sql);
            remove(lst.front().c_str());
        }

        // 删除该用户分享这条文件的记录
        sprintf(sql,"delete from shared_file where u_id = %lld and f_id = %lld",psdr->userId, psdr->fileId);
        m_SqlUtils->Update(sql);
    }
}



void TCPKernel::addFriendRQ(SOCKET sock, char *buf)
{
    STRU_ADDFRIEND_RQ* psar = (STRU_ADDFRIEND_RQ*)buf;
    STRU_ADDFRIEND_RS sar{};
    sar.m_ret = ADDFRIEND_NO_USER;
    sar.m_friendId = 0;
    sar.m_friendName[0] = '\0';

    char sql[SQLLEN] = {0};
    sprintf(sql, "select u_id from user where u_name = '%s'", psar->m_friendName);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);

    if (!lst.empty())
    {
        long long friendId = atoll(lst.front().c_str());
        lst.pop_front();
        if (friendId == psar->m_userId)
        {
            sar.m_ret = ADDFRIEND_EXIST;
        }
        else
        {
            // 双向插入好友关系
            sprintf(sql, "insert into friend(u_id1, u_id2) values(%lld, %lld)", psar->m_userId, friendId);
            if (m_SqlUtils->Update(sql))
            {
                sprintf(sql, "insert into friend(u_id1, u_id2) values(%lld, %lld)", friendId, psar->m_userId);
                m_SqlUtils->Update(sql);

                sar.m_ret = ADDFRIEND_SUC;
                sar.m_friendId = friendId;
                strncpy(sar.m_friendName, psar->m_friendName, MAXSIZE - 1);
                sar.m_friendName[MAXSIZE - 1] = '\0';

                // 如果对方此时也在线，实时通知对方刷新好友列表
                if (isOnline(friendId))
                {
                    STRU_GETFRIENDLIST_RQ req;
                    req.m_userId = friendId;
                    std::lock_guard<std::mutex> lk(m_mutexOnline);
                    auto pos = m_mapIdToSock.find(friendId);
                    if (pos != m_mapIdToSock.end())
                        getFriendListRQ(pos->second, (char*)&req);
                }
            }
            else
            {
                sar.m_ret = ADDFRIEND_EXIST;
            }
        }
    }
    m_TCPNet->SendData(sock, (char*)&sar, sizeof(sar));

    // 告知添加者：对方当前是否在线
    if (sar.m_ret == ADDFRIEND_SUC)
    {
        STRU_FRIENDSTATUS_RS st;
        st.m_friendId = sar.m_friendId;
        st.m_online = isOnline(sar.m_friendId) ? 1 : 0;
        m_TCPNet->SendData(sock, (char*)&st, sizeof(st));
    }
}

void TCPKernel::getFriendListRQ(SOCKET sock, char *buf)
{
    STRU_GETFRIENDLIST_RQ* psgr = (STRU_GETFRIENDLIST_RQ*)buf;
    char sql[SQLLEN] = {0};
    sprintf(sql, "select u.u_id, u.u_name from friend f join user u on f.u_id2 = u.u_id where f.u_id1 = %lld", psgr->m_userId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 2, lst);
    printf("friend list request: u_id=%lld, friends=%d\n", psgr->m_userId, (int)(lst.size() / 2));

    STRU_GETFRIENDLIST_RS sgr;
    int i = 0;
    while (!lst.empty())
    {
        std::string fId = lst.front(); lst.pop_front();
        std::string fName = lst.front(); lst.pop_front();

        sgr.m_friendInfo[i].m_friendId = atoll(fId.c_str());
        strncpy(sgr.m_friendInfo[i].m_friendName, fName.c_str(), MAXSIZE - 1);
        sgr.m_friendInfo[i].m_friendName[MAXSIZE - 1] = '\0';
        sgr.m_friendInfo[i].m_online = isOnline(sgr.m_friendInfo[i].m_friendId) ? 1 : 0;
        i++;
        if (i == FILENUM || lst.empty())
        {
            sgr.m_friendNum = i;
            m_TCPNet->SendData(sock, (char*)&sgr, sizeof(sgr));
            i = 0;
            memset(sgr.m_friendInfo, 0, sizeof(sgr.m_friendInfo));
        }
    }
}

void TCPKernel::chatRQ(SOCKET sock, char *buf)
{
    STRU_CHAT_RQ* pscr = (STRU_CHAT_RQ*)buf;
    // 查询发送者的用户名填入包中
    char sql[SQLLEN] = {0};
    sprintf(sql, "select u_name from user where u_id = %lld", pscr->m_sendUserId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);
    if (!lst.empty())
    {
        strncpy(pscr->m_sendUserName, lst.front().c_str(), MAXSIZE - 1);
        pscr->m_sendUserName[MAXSIZE - 1] = '\0';
    }

    // 查找接收者是否在线
    if (sendToUser(pscr->m_recvUserId, (char*)pscr, sizeof(STRU_CHAT_RQ)))
    {
        // 在线，已转发
    }
    else
    {
        // 不在线（或发送失败），回复发送者
        STRU_CHAT_RS crs;
        crs.m_ret = CHAT_OFFLINE;
        crs.m_recvUserId = pscr->m_recvUserId;
        m_TCPNet->SendData(sock, (char*)&crs, sizeof(crs));
    }
}

bool TCPKernel::sendToUser(long long userId, char* buf, int len)
{
    // 锁内完成「查表 + 发送」，同时 onSocketClosed 的关闭操作也在锁内，
    // 避免「A 线程正给 B 发数据，B 线程刚好 closesocket」这种竞争
    std::lock_guard<std::mutex> lk(m_mutexOnline);
    auto it = m_mapIdToSock.find(userId);
    if (it == m_mapIdToSock.end())
        return false;
    return m_TCPNet->SendData(it->second, buf, len);
}

bool TCPKernel::isOnline(long long userId)
{
    std::lock_guard<std::mutex> lk(m_mutexOnline);
    return m_mapIdToSock.find(userId) != m_mapIdToSock.end();
}

void TCPKernel::pushFriendStatus(long long userId, int online)
{
    // 查出这个用户的所有好友
    char sql[SQLLEN] = {0};
    sprintf(sql, "select u_id2 from friend where u_id1 = %lld", userId);
    std::list<std::string> lst;
    m_SqlUtils->Select(sql, 1, lst);

    STRU_FRIENDSTATUS_RS rs;
    rs.m_friendId = userId;
    rs.m_online = online;

    // 锁内发送（发送失败直接忽略）
    std::lock_guard<std::mutex> lk(m_mutexOnline);
    for (auto it = lst.begin(); it != lst.end(); ++it)
    {
        long long fid = atoll(it->c_str());
        auto pos = m_mapIdToSock.find(fid);
        if (pos != m_mapIdToSock.end())
            m_TCPNet->SendData(pos->second, (char*)&rs, sizeof(rs));
    }
}

void TCPKernel::onSocketClosed(SOCKET sock)
{
    TCPKernel* pThis = (TCPKernel*)getKernel();
    if (!pThis) return;

    long long userId = 0;
    {
        std::lock_guard<std::mutex> lk(pThis->m_mutexOnline);
        auto it = pThis->m_mapSockToId.find(sock);
        if (it != pThis->m_mapSockToId.end())
        {
            userId = it->second;
            pThis->m_mapSockToId.erase(it);
            pThis->m_mapIdToSock.erase(userId);
        }
        // 在同一把锁内关闭，保证其他线程不会正在往这个 socket 发数据
        closesocket(sock);
    }

    printf("connection closed: sock=%lld, user=%lld\n", (long long)sock, userId);

    // 通知好友：该用户下线了
    if (userId != 0)
        pThis->pushFriendStatus(userId, 0);
}



























































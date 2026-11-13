#include "mysqlutils.h"

MysqlUtils::MysqlUtils()
{
    sock = new MYSQL;
    mysql_init(sock);
    mysql_set_character_set(sock, "utf8");
}

MysqlUtils::~MysqlUtils()
{
    if(sock)
    {
        delete sock;
        sock = nullptr;
    }
}

bool MysqlUtils::ConnectMysql(const char *host, const char *user, const char *passwd, const char *db)
{
    if(!mysql_real_connect(sock, host, user, passwd,db,3306,0,0))
    {
        return false;
    }
    m_host.clear();
    m_user.clear();
    m_pwd.clear();
    m_db.clear();
    m_host.append(host);
    m_user.append(user);
    m_pwd.append(passwd);
    m_db.append(db);
    std::thread heart(HeadrtBeatLoop, this);
    heart.detach();

    return true;
}

void MysqlUtils::DisConnect()
{
    mysql_close(sock);
}

bool MysqlUtils::Select(const char *sql, int column, std::list<std::string>& lstStr)
{
    m_mutex.lock();
    if(mysql_query(sock, sql))
    {
        m_mutex.unlock();
        return false;
    }
    results = mysql_store_result(sock);
    if (results == nullptr)
    {
        m_mutex.unlock();
        return false;
    }
    while(record = mysql_fetch_row(results))
    {
        for(int i = 0; i < column; i++)
        {
            if(record[i] == nullptr)
            {
                record[i] = (char*)"null";
            }
            lstStr.push_back((record[i]));
        }
    }
    m_mutex.unlock();
    return true;
}

bool MysqlUtils::Update(const char *sql)
{
    m_mutex.lock();
    if(mysql_query(sock, sql))
    {
        m_mutex.unlock();
        return false;
    }
    m_mutex.unlock();
    return true;
}

void MysqlUtils::HeadrtBeatLoop(MysqlUtils* pThis)
{
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(LOOPTIME));
        pThis->m_mutex.lock();
        if(mysql_ping(pThis->sock) != 0)
        {
            printf("mysql ping err\n");
            pThis->DisConnect();
            if (!mysql_real_connect(pThis->sock, pThis->m_host.c_str(),
                                    pThis->m_user.c_str(),pThis->m_pwd.c_str(),
                                    pThis->m_db.c_str(),3306,0,0))
            {
                printf("reset mysql connect err\n");
            }
            else
            {
                printf("reset mysql connect suecces\n");
            }
        }
        pThis->m_mutex.unlock();
    }
}

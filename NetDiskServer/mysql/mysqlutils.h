#ifndef MYSQLUTILS_H
#define MYSQLUTILS_H

#include <mysql.h>
#include <list>
#include <string>
#include <thread>
#include <mutex>

#define LOOPTIME    (10 * 1000)

class MysqlUtils
{
public:
    MysqlUtils();
    ~MysqlUtils();
public:
    bool ConnectMysql(const char* host, const char* user, const char* passwd, const char* db);
    void DisConnect();
    bool Select(const char* sql, int column, std::list<std::string>& lstStr);
    bool Update(const char* sql);

private:
    MYSQL* sock;
    MYSQL_RES* results;
    MYSQL_ROW record;
    std::string m_host,m_user,m_pwd,m_db;
    static void HeadrtBeatLoop(MysqlUtils* );
    std::mutex m_mutex;
};

#endif // MYSQLUTILS_H

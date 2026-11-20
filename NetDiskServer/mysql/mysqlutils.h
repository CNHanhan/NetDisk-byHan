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
    // MySQL C API 的多线程要求：库初始化一次，每个线程各自 init/end
    static void libraryInit();
    static void threadInit();
    static void threadEnd();

    bool ConnectMysql(const char* host, const char* user, const char* passwd, const char* db);
    void DisConnect();
    bool Select(const char* sql, int column, std::list<std::string>& lstStr);
    bool Update(const char* sql);

private:
    // 主连接：仅用于启动时校验参数 + 心跳保活
    MYSQL* m_main;
    static void HeadrtBeatLoop(MysqlUtils*);
    std::mutex m_mutex;
};

#endif // MYSQLUTILS_H

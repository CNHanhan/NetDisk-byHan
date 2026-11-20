#include "mysqlutils.h"
#include "../config.h"
#include <cstdio>
#include <mutex>

// ==================== 线程私有连接 ====================
// 每个使用数据库的线程各自持有一个 MYSQL 连接。
// 这样彻底避免「多个线程共享同一个 MYSQL 句柄」带来的堆损坏（c0000374）问题。
static thread_local MYSQL*  g_threadSock   = nullptr;
static thread_local bool    g_threadInited = false;

// 连接参数（由 ConnectMysql 填写，供各线程创建自己的连接）
static std::string g_host, g_user, g_pwd, g_db;
static bool        g_paramsReady = false;

static MYSQL* threadConn()
{
    if (g_threadSock)
        return g_threadSock;

    if (!g_paramsReady)
        return nullptr;

    if (!g_threadInited)
    {
        mysql_thread_init();
        g_threadInited = true;
    }

    MYSQL* conn = mysql_init(nullptr);
    if (!conn)
        return nullptr;

    mysql_set_character_set(conn, "utf8");

    if (!mysql_real_connect(conn, g_host.c_str(), g_user.c_str(), g_pwd.c_str(),
                            g_db.c_str(), MYSQL_PORT, 0, 0))
    {
        fprintf(stderr, "thread mysql connect error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return nullptr;
    }

    g_threadSock = conn;
    return g_threadSock;
}

static void closeThreadConn()
{
    if (g_threadSock)
    {
        mysql_close(g_threadSock);
        g_threadSock = nullptr;
    }
}

// ==================== MysqlUtils ====================

MysqlUtils::MysqlUtils()
{
    m_main = mysql_init(nullptr);
    if (m_main)
        mysql_set_character_set(m_main, "utf8");
}

MysqlUtils::~MysqlUtils()
{
    if (m_main)
    {
        mysql_close(m_main);
        m_main = nullptr;
    }
}

void MysqlUtils::libraryInit()
{
    mysql_library_init(0, nullptr, nullptr);
}

void MysqlUtils::threadInit()
{
    if (!g_threadInited)
    {
        mysql_thread_init();
        g_threadInited = true;
    }
}

void MysqlUtils::threadEnd()
{
    closeThreadConn();
    if (g_threadInited)
    {
        mysql_thread_end();
        g_threadInited = false;
    }
}

bool MysqlUtils::ConnectMysql(const char *host, const char *user, const char *passwd, const char *db)
{
    g_host = host;
    g_user = user;
    g_pwd  = passwd;
    g_db   = db;

    if(!m_main)
    {
        fprintf(stderr, "mysql_init failed\n");
        return false;
    }

    // 用主连接验证参数是否正确，失败时能打印出具体原因
    if(!mysql_real_connect(m_main, host, user, passwd, db, MYSQL_PORT, 0, 0))
    {
        fprintf(stderr, "mysql connect error: %s\n", mysql_error(m_main));
        return false;
    }

    g_paramsReady = true;

    std::thread heart(HeadrtBeatLoop, this);
    heart.detach();
    return true;
}

void MysqlUtils::DisConnect()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_main)
    {
        mysql_close(m_main);
        m_main = nullptr;
    }
}

bool MysqlUtils::Select(const char *sql, int column, std::list<std::string>& lstStr)
{
    MYSQL* conn = threadConn();
    if (!conn)
        return false;

    if (mysql_query(conn, sql))
    {
        fprintf(stderr, "mysql query error: %s | sql=%s\n", mysql_error(conn), sql);
        return false;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr)
        return false;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        for (int i = 0; i < column; i++)
        {
            // 只读，不修改 row（那是 libmysql 拥有的内存）
            lstStr.push_back(row[i] ? row[i] : "null");
        }
    }

    mysql_free_result(res);
    return true;
}

bool MysqlUtils::Update(const char *sql)
{
    MYSQL* conn = threadConn();
    if (!conn)
        return false;

    if (mysql_query(conn, sql))
    {
        fprintf(stderr, "mysql update error: %s | sql=%s\n", mysql_error(conn), sql);
        return false;
    }

    // 语句若带回结果集，需要清理，避免影响同一连接上的下一次查询
    MYSQL_RES* res = mysql_store_result(conn);
    if (res)
        mysql_free_result(res);

    return true;
}

void MysqlUtils::HeadrtBeatLoop(MysqlUtils* pThis)
{
    for(;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(LOOPTIME));

        std::lock_guard<std::mutex> lk(pThis->m_mutex);

        if (!pThis->m_main)
            continue;

        if (mysql_ping(pThis->m_main) == 0)
            continue;

        printf("mysql ping err, reconnecting...\n");
        fflush(stdout);

        // mysql_close 之后不能复用同一个 MYSQL*，必须重新 init
        mysql_close(pThis->m_main);
        pThis->m_main = mysql_init(nullptr);
        if (!pThis->m_main)
        {
            printf("reset mysql init err\n");
            continue;
        }
        mysql_set_character_set(pThis->m_main, "utf8");

        if (!mysql_real_connect(pThis->m_main, g_host.c_str(), g_user.c_str(), g_pwd.c_str(),
                                g_db.c_str(), MYSQL_PORT, 0, 0))
        {
            printf("reset mysql connect err: %s\n", mysql_error(pThis->m_main));
        }
        else
        {
            printf("reset mysql connect success\n");
        }
    }
}

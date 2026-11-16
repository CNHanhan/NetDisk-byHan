#ifndef CONFIG_H
#define CONFIG_H

// ==================== 服务端可配置项 ====================
// 部署到自己的机器时，只需要改这一个文件

// ---- MySQL ----
#define MYSQL_HOST          "127.0.0.1"
#define MYSQL_PORT          3306
#define MYSQL_USER          "root"
#define MYSQL_PASSWD        "root"          // TODO: 改成你自己的密码
#define MYSQL_DB            "disk"

// ---- 监听 ----
#define SERVER_LISTEN_IP    "0.0.0.0"       // 0.0.0.0 = 监听所有网卡，云服务器必须这样
#define SERVER_LISTEN_PORT  8899

// ---- 网盘物理文件根目录（末尾的 '/' 不能少）----
#ifdef _WIN32
#define FILEPATHBASE        "D:/disk/"
#else
#define FILEPATHBASE        "/home/ubuntu/disk/"
#endif

// ---- 平台差异补齐 ----
#ifndef _WIN32
#ifndef MAX_PATH
#define MAX_PATH            260
#endif
#endif

#endif // CONFIG_H

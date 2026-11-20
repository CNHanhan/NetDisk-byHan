// 服务端入口：不依赖 Qt，Windows 下可用 Qt Creator 编译，
// Linux 下可直接用 g++ 一行命令编译运行（见 README）
#include "kernel/tcpkernel.h"
#include "mysql/mysqlutils.h"
#include <cstdio>
#include <thread>
#include <chrono>
#include <csignal>

#ifdef _WIN32
#include <windows.h>

// 崩溃时打印异常码和地址，否则 Qt Creator 只会显示 "terminated abnormally"
static LONG WINAPI crashHandler(EXCEPTION_POINTERS* info)
{
    printf("\n!!! unhandled exception: code=0x%08lX, address=%p\n",
           (unsigned long)info->ExceptionRecord->ExceptionCode,
           info->ExceptionRecord->ExceptionAddress);
    fflush(stdout);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

#ifdef _WIN32
    SetUnhandledExceptionFilter(crashHandler);
#else
    // Linux：对已关闭的 socket 写入会触发 SIGPIPE，默认动作是杀进程。
    // 忽略它，send 会正常返回 -1（EPIPE），由业务层处理。
    signal(SIGPIPE, SIG_IGN);
#endif

    // MySQL 多线程要求：先用 mysql_library_init 初始化库，再创建任何线程
    MysqlUtils::libraryInit();

    IKernel* p = TCPKernel::getKernel();
    if(p->open())
    {
        printf("server is running....\n");
        fflush(stdout);
    }
    else
    {
        printf("server is err....\n");
        return 1;
    }

    // 阻塞主线程，工作都交给各连接的处理线程
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3600));
    }
    return 0;
}

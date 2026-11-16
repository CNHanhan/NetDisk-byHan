// 服务端入口：不依赖 Qt，Windows 下可用 Qt Creator 编译，
// Linux 下可直接用 g++ 一行命令编译运行（见 README）
#include "kernel/tcpkernel.h"
#include <cstdio>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

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

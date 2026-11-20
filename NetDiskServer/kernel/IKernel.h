#ifndef IKERNEL_H
#define IKERNEL_H
#include "../server/INet.h"

class IKernel
{
public:
    IKernel(){}
    virtual ~IKernel(){}
public:
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual void dealData(SOCKET sock, char* buf, int len) = 0;
};

#endif // IKERNEL_H

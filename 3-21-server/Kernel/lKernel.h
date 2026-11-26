#ifndef LKERNEL_H
#define LKERNEL_H
#include <winsock2.h>
class IKernel{
public:
    IKernel(){}
    virtual ~IKernel(){}
public:
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual void dealData(SOCKET socketWaiter,const char* szbuf) = 0;
};

#endif // LKERNEL_H

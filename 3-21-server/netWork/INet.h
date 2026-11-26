#ifndef INET_H
#define INET_H
#include <winsock2.h>

class INet{
public:
    INet(){}
    virtual ~INet(){}
public:
public:
    virtual bool InitNetWork(unsigned long dwip = 0,short nport = 8899) = 0;
    virtual void UnitNetWork() = 0;
    virtual bool SendData(SOCKET sockWaiter,const char * szbuf ,int nLen) = 0;
    virtual void RecvData() = 0;
};

#endif // INET_H

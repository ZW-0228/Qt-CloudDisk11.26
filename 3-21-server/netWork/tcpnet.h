#ifndef TCPNET_H
#define TCPNET_H
#pragma once
#include "INet.h"
#include <cstdio>
#include <list>
#include <map>
#include <iostream>
#include "INet.h"
#include "../Kernel/kernel.h"
using namespace std;
class TCPNet : public INet
{
public:
    TCPNet();
    ~TCPNet();
public:
    bool InitNetWork(unsigned long dwip = 0,short nport = 8899);
    void UnitNetWork();
    bool SendData(SOCKET sockWaiter,const char * szbuf ,int nLen);
    void RecvData();
public:
    static DWORD WINAPI ThreadSelect(LPVOID lp);
    static DWORD WINAPI ThreadRecv(LPVOID lp);
private:
    SOCKET m_sockListen;
    list<HANDLE> m_lstHandle;
    bool m_bQuitFlag;
    map<DWORD,SOCKET> m_mapThreadToSocket;
    list<SOCKET> m_lstSocket;
    fd_set m_fdsets;
};



#endif // TCPNET_H

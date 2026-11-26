#include "tcpnet.h"

TCPNet::TCPNet() {
    m_sockListen = 0;
    m_bQuitFlag = true;
    //FD 是 File Descriptor 的缩写，中文译作文件描述符。
    FD_ZERO(&m_fdsets);      //清空套接字集合
}

TCPNet::~TCPNet()
{
    closesocket(m_sockListen);
    WSACleanup();
}

bool TCPNet::InitNetWork(unsigned long dwip, short nport)
{
    //1
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return false;
    }
    else
        printf("server The Winsock 2.2 dll was found okay\n");
    //2
    m_sockListen = socket(AF_INET,SOCK_STREAM,0);      //tcp
    if(m_sockListen == INVALID_SOCKET){
        printf("socket err\n");
        closesocket(m_sockListen);
        WSACleanup();
        return false;
    }
    //3
    sockaddr_in addrserver;
    addrserver.sin_family = AF_INET;
    addrserver.sin_port = htons(nport);    //返回端口号
    addrserver.sin_addr.S_un.S_addr = dwip;   //接收所有ip地址产生的信息
    if(SOCKET_ERROR == bind(m_sockListen,(sockaddr*)&addrserver,sizeof(addrserver))){
        printf("bind err\n");
        closesocket(m_sockListen);
        WSACleanup();
        return false;
    }
    //4  监听
    if(SOCKET_ERROR == listen(m_sockListen,128)){   //程序  运行在  用户区  listen  监听  内核区
        printf("listen err\n");
        closesocket(m_sockListen);
        WSACleanup();
        return false;
    }
    HANDLE hThread = CreateThread(0,0,&ThreadSelect,this,0,0);   //安全属性 初始堆栈大小  线程函数地址  传给线程函数的值  标志  线程ID(out)
    if(hThread){   //这个线程中  一直处理select返回值
        m_lstHandle.push_back(hThread);
    }
    // hThread = CreateThread(0,0,&ThreadRecv,this,0,0);
    // if(hThread){
    //   m_lstHandle.push_back(hThread);
    // }
    FD_SET(m_sockListen,&m_fdsets);
    return true;
}

void TCPNet::UnitNetWork()
{
    m_bQuitFlag = false;
    auto ite = m_lstHandle.begin();
    while(ite!=m_lstHandle.end()){
        if(WAIT_TIMEOUT == WaitForSingleObject(*ite,100)){
            TerminateThread(*ite,-1);
        }
        CloseHandle(*ite);
        *ite = NULL;
        ite++;
    }
    m_lstHandle.clear();
}

bool TCPNet::SendData(SOCKET sockWaiter, const char *szbuf, int nLen)
{
    //校验参数
    if(!szbuf || nLen <= 0||sockWaiter == INVALID_SOCKET){
        return false;
    }
    //包大小
    if(send(sockWaiter,(char*)&nLen,sizeof(int),0)<=0)
        return false;
    //包内容
    if(send(sockWaiter,szbuf,nLen,0)<=0)
        return false;
    return true;
}

void TCPNet::RecvData()
{
    for(auto ite = m_lstSocket.begin();ite!=m_lstSocket.end();ite++){
        SOCKET sockWaiter = *ite;
        int nPackageSize;
        //先接收包大小
        int nRecvNum = recv(sockWaiter,(char*)&nPackageSize,sizeof(int),0);

        if(nRecvNum<=0){    //可能是客户端没有消息产生   或者是客户端关闭
            if(GetLastError() == 10054){       //客户端关闭连接    WSAECONNRESET
                closesocket(*ite);
                ite = m_lstSocket.erase(ite);    //返回被删除元素的后一个元素的迭代器，避免迭代器失效
            }
            continue;
        }
        //再接收包内容
        char *pszbuf = new char[nPackageSize];
        int noffset = 0;
        while(nPackageSize){
            nRecvNum = recv(sockWaiter,pszbuf + noffset,nPackageSize,0);
            noffset += nRecvNum;
            nPackageSize -= nRecvNum;
        }
        cout<<pszbuf<<endl;
        delete[] pszbuf;
        pszbuf = nullptr;
    }
    Sleep(10);
}

DWORD TCPNet::ThreadSelect(LPVOID lp)
{
    TCPNet* pthis = (TCPNet*)lp;
    TIMEVAL tv;
    tv.tv_sec=0;
    tv.tv_usec=100;
    sockaddr_in addrclient;
    int nLen;
    int Num;
    int nReadNum;
    int nPackSize;
    char* pszbuf;
    while(pthis->m_bQuitFlag){
        //判断哪个套接字在我的 select 返回的集合中
        //定义临时集合
        fd_set fdtemp;
        fdtemp = pthis->m_fdsets;

        //回去搜索select函数  弄懂参数
        //每次在内存中查看fdtemp集合 中哪些套接字发生了可读的网络事件，一次查看tv(100)微秒，返回值是有几个套接字发生了网络事件
        Num = select(fdtemp.fd_count,&fdtemp,0,0,&tv);  //代表有几个客户端产生了网络事件
        while(Num>0){   //每循环一次 处理一个网络事件
            if(FD_ISSET(pthis->m_sockListen,&fdtemp)){
                //有客户端来连接
                nLen = sizeof(addrclient);
                SOCKET sockWaiter = accept(pthis->m_sockListen,(sockaddr*)&addrclient,&nLen);   //是一个阻塞函数  当连接队列中没有套接字  将一直阻塞
                if(sockWaiter == INVALID_SOCKET){
                     continue;
                }
                cout<<"clent ip:" << inet_ntoa(addrclient.sin_addr)<<"port:"<<addrclient.sin_port<<endl;
                //Waiter  代表着一个客户端
                pthis->m_lstSocket.push_back(sockWaiter);
                FD_SET(sockWaiter,&pthis->m_fdsets);
                FD_CLR(pthis->m_sockListen,&fdtemp);
                Num--;
            }else{
                //判断Waiter 在不在集合中    可以遍历Waiter链表  一一比较每一个Waiter
                //证明客户端发来消息了
                for(auto ite = pthis->m_lstSocket.begin();ite!=pthis->m_lstSocket.end();ite++){
                    if(FD_ISSET(*ite,&fdtemp)){
                        SOCKET sockWaiter = *ite;
                        nReadNum = recv(sockWaiter,(char*)&nPackSize,sizeof(int),0);
                        if(nReadNum<=0){
                            Num--;
                            if(GetLastError() == 10054){
                                //客户端下线了
                                closesocket(sockWaiter);
                                FD_CLR(*ite,&pthis->m_fdsets);
                                ite = pthis->m_lstSocket.erase(ite);
                            }
                            continue;
                        }
                        int noffset = 0;
                        pszbuf = new char[nPackSize];
                        while(nPackSize>0){
                            nReadNum = recv(sockWaiter,pszbuf+noffset,nPackSize,0);
                            noffset+=nReadNum;
                            nPackSize-=nReadNum;
                        }
                        //cout<<"client say:"<<pszbuf<<endl;
                        Kernel::GetKernel()->dealData(*ite,pszbuf);
                        delete[] pszbuf;
                        pszbuf = nullptr;
                        Num--;
                    }
                }
            }
        }
    }
    return 0;
}

DWORD TCPNet::ThreadRecv(LPVOID lp)
{
    //轮询查看所有客户端，谁有消息，处理谁
    TCPNet* pthis = (TCPNet*)lp;

    while(pthis->m_bQuitFlag){
        pthis->RecvData();
    }
    return 0;
}

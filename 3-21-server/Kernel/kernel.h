#ifndef KERNEL_H
#define KERNEL_H
#pragma once    //只编译一次  防止文件重复引入
#include "lKernel.h"
#include "../netWork/tcpnet.h"
#include <QDebug>
#include "../CMySQL/cmysql.h"
#include "../packdef.h"
struct fileinfo{
    FILE* pFile;
    long long userId;
    long long fileId;
    long long fileSize;
    long long pos;
};

class Kernel:public IKernel
{
    //构造函数
    //拷贝构造函数
private:
    Kernel();
    Kernel(Kernel& pk){}
public:
    ~Kernel();
public:
    static Kernel* GetKernel();
    bool open();
    void close();
    void dealData(SOCKET socketWaiter,const char* szbuf);

    void RegisterRq(SOCKET socketWaiter,const char* szbuf);
    void LoginRq(SOCKET socketWaiter,const char* szbuf);
    void GetFileListRq(SOCKET socketWaiter,const char* szbuf);
    void UploadFileInfoRq(SOCKET socketWaiter,const char* szbuf);
    void UploadFileContentRq(SOCKET socketWaiter,const char* szbuf);
    void SelectFileRq(SOCKET socketWaiter,const char* szbuf);
    void DeleteFileRq(SOCKET socketWaiter,const char* szbuf);
    void SharedLinkRq(SOCKET socketWaiter,const char* szbuf);
    void GetLinkRq(SOCKET socketWaiter,const char* szbuf);
    void DownloadFileInfoRq(SOCKET socketWaiter,const char* szbuf);

private:
    static Kernel* m_pKernel;
    INet* m_pNet;
    CMySql* m_pSql;
    char m_szSystemPath[FILE_PATH];
    list<fileinfo*> m_lstFileInfo;
};

#endif // KERNEL_H

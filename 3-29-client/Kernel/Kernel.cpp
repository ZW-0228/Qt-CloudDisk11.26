#include "kernel.h"

Kernel::Kernel(QObject *parent)
    :QObject{parent}
{
    m_pNet = new tcpnet(this);
}

Kernel::~Kernel()
{
    DisConnect();
    delete m_pNet;
    m_pNet = NULL;
}
bool Kernel::Connect(const char *szip, short nport)
{
    if(m_pNet->ConnectServer(szip,nport))
        return true;
    return false;
}

void Kernel::DisConnect()
{
    m_pNet->disConnectServer();
}

bool Kernel::SendData(const char *szbuf, int nLen)
{
    if(m_pNet->SendData(szbuf,nLen))
        return true;
    return false;
}

void Kernel::DealData(const char *szbuf)
{
    switch(*szbuf){
    case _default_protocol_login_rs:
        emit loginRs(szbuf);
        break;
    case _default_protocol_register_rs:
        emit RegisterRs(szbuf);
        break;
    case _default_protocol_getfilelist_rs:
        emit GetFileListRs(szbuf);
        break;
    case _default_protocol_uploadfileinfo_rs:
        emit UploadFileInfoRs(szbuf);
        break;
    case _default_protocol_selectfile_rs:
        emit SelectFileRs(szbuf);
        break;
    case _default_protocol_sharelink_rs:
        emit SharedLinkRs(szbuf);
    case _default_protocol_getlink_rs:
        emit GetLinkRs(szbuf);
        break;
    case _default_protocol_downloadfilecontent_rs:
        emit DownLoadFileRs(szbuf);
        break;
    }
}

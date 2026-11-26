#include "kernel.h"
Kernel* Kernel::m_pKernel = new Kernel;
Kernel::Kernel()
{
    m_pNet = new TCPNet;
    m_pSql = new CMySql;
    strcpy(m_szSystemPath,"D:/Qtprogram/disk_file/");
}

Kernel::~Kernel()
{
    delete m_pNet;
    m_pNet = NULL;
    delete m_pSql;
    m_pSql = NULL;
}

Kernel *Kernel::GetKernel()//懒汉式  不支持线程安全
{
    // if(m_pKernel == nullptr)
    //     m_pKernel = new Kernel;
    // return m_pKernel;
    //饿汉式  支持线程安全
    return m_pKernel;
}

bool Kernel::open()
{
    if(!m_pNet->InitNetWork()){
        qDebug()<<"net error";
    return false;
    }
    if(!m_pSql->ConnectMysql("127.0.0.1","root","123456","3-22disk")){
        qDebug()<<"mysql error";
        return false;
    }
    return true;
}


void Kernel::close()
{
    m_pNet->UnitNetWork();
    m_pSql->DisConnect();
}

void Kernel::dealData(SOCKET socketWaiter, const char *szbuf)
{
    switch(*szbuf){
    case _default_protocol_register_rq:
        //客户端产生的是注册请求
        RegisterRq(socketWaiter,szbuf);
        break;
    case _default_protocol_login_rq:
        LoginRq(socketWaiter,szbuf);
        break;
    case _default_protocol_getfilelist_rq:
        GetFileListRq(socketWaiter,szbuf);
        break;
    case _default_protocol_uploadfileinfo_rq:
        UploadFileInfoRq(socketWaiter,szbuf);
        break;
    case _default_protocol_uploadfilecontent_rq:
        UploadFileContentRq(socketWaiter,szbuf);
        break;
    case _default_protocol_selectfile_rq:
        SelectFileRq(socketWaiter,szbuf);
        break;
    case _default_protocol_deletefile_rq:
        DeleteFileRq(socketWaiter,szbuf);
        break;
    case _default_protocol_sharelink_rq:
        SharedLinkRq(socketWaiter,szbuf);
        break;
    case _default_protocol_getlink_rq:
        GetLinkRq(socketWaiter,szbuf);
        break;
    case _default_protocol_downloadfileinfo_rq:
        DownloadFileInfoRq(socketWaiter,szbuf);
        break;
    }
}

void Kernel::RegisterRq(SOCKET socketWaiter, const char *szbuf)
{
    //解析请求包中用户名与密码，手机号
    //写到数据库中
        //成功与失败  通过注册回复包 返回给客户端
    STRU_REGISTER_RQ* srr = (STRU_REGISTER_RQ*)szbuf;
    STRU_REGISTER_RS srs;
    srs.szResult = _register_res_failed;
    list<string> lsStr;
    char szsql[SQLLEN];
    char FilePath[FILE_PATH];
    sprintf(szsql,"insert into user(u_name,u_password,u_tel) values('%s','%s',%lld)",
            srr->szName,srr->szPassword,srr->szTel);
    if(m_pSql->UpdateMysql(szsql)){
        //注册成功
        srs.szResult = _register_res_success;
        //获取用户id   根据id创建文件夹
        sprintf(szsql,"select u_id from user where u_name = '%s' and u_password = '%s'",
                srr->szName,srr->szPassword);
        m_pSql->SelectMysql(szsql,1,lsStr);
        if(lsStr.size()>0){
            //为当前用户创建一个文件夹
            string UserId = lsStr.front();
            lsStr.pop_front();

            sprintf(FilePath,"%s%s",m_szSystemPath,UserId.c_str());
            CreateDirectoryA(FilePath,0);   //创建文件夹函数
        }
    }
    m_pNet->SendData(socketWaiter,(char*)&srs,sizeof(srs));
}

void Kernel::LoginRq(SOCKET socketWaiter, const char *szbuf)
{
    //根据用户名查询密码，三种
    STRU_LOGIN_RQ* slr = (STRU_LOGIN_RQ*)szbuf;
    STRU_LOGIN_RS sls;
    sls.szResult = _login_res_noexist;
    char szsql[SQLLEN] = {0};
    list<string>lsStr;
    sprintf(szsql,"select u_id,u_password from user where u_name = '%s'",slr->szName);
    m_pSql->SelectMysql(szsql,2,lsStr);
    if(lsStr.size() > 0){
        //证明用户是存在的
        //密码比较
        string UserId = lsStr.front();
        lsStr.pop_front();

        string password = lsStr.front();
        lsStr.pop_front();

        sls.szResult = _login_res_failed;
        if(strcmp(password.c_str(),slr->szPassword) == 0){
            //用户存在且密码正确
            sls.szResult = _login_res_success;
            sls.szUserId = _atoi64(UserId.c_str());
        }
    }
    //返回给客户端
    m_pNet->SendData(socketWaiter,(char*)&sls,sizeof(sls));
}

void Kernel::GetFileListRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_GETFILELIST_RQ* sgr = (STRU_GETFILELIST_RQ*)szbuf;
    STRU_GETFILELIST_RS sgrs;
    char szsql[SQLLEN] = {0};
    int index = 0;
    list<string> lsStr;
    //看是哪个人获取文件信息
    sprintf(szsql,"select f_name,f_size,f_uploadtime from ufile where u_id = %lld;",sgr->szUserId);
    //数据查询此人上传过的文件
     m_pSql->SelectMysql(szsql,3,lsStr);
    //封装进GetFileListRS结构体中
     while(lsStr.size()>0){
         string fileName = lsStr.front();
         lsStr.pop_front();

         string fileSize = lsStr.front();
         lsStr.pop_front();

         string fileTime = lsStr.front();
         lsStr.pop_front();

         //放入文件信息数组的哪个？
         strcpy(sgrs.arrFileInfo[index].szFileName,fileName.c_str());
         strcpy(sgrs.arrFileInfo[index].szFileUploadTime,fileTime.c_str());
         sgrs.arrFileInfo[index].szFileSize = atoll(fileSize.c_str());
         index++;
         if(index == FILE_NUM || lsStr.size() == 0){    //文件信息数组中已经包含15个文件   链表所有数据已经取出来了
             sgrs.szFileNume = index;
             //返回给客户端
             m_pNet->SendData(socketWaiter,(char*)&sgrs,sizeof(sgrs));
             ZeroMemory(sgrs.arrFileInfo,sizeof(sgrs.arrFileInfo)); //内存清空
             index = 0;
         }
     }
}

void Kernel::UploadFileInfoRq(SOCKET socketWaiter, const char *szbuf)
{
    //1、校验数据库中是否存在文件
    STRU_UPLOADFILEINFO_RQ* sur = (STRU_UPLOADFILEINFO_RQ*)szbuf;
    char szsql[SQLLEN] = {0};
    list<string> lsStr;
    STRU_UPLOADFILEINFO_RS surs;
    surs.m_pos = 0;
    strcpy(surs.szFileMD5,sur->szFileMD5);
    sprintf(szsql,"select u_id,f_id from ufile where f_MD5 = '%s'",sur->szFileMD5);
    m_pSql->SelectMysql(szsql,2,lsStr);
    bool flag = false;
    string FileId;
    auto ite = lsStr.begin();
    while(ite != lsStr.end()){
        string UserId = *ite;
        long long Uid = atoll(UserId.c_str());
        ite++;
        FileId = *ite;
        ite++;
        if(Uid==sur->UserId){
            flag = true;
            //1、自己上传的 重复上传
            surs.m_Result = _uploadfileinfo_repeat;
            surs.fileid = atoll(FileId.c_str());
            //链表
            auto fileite = m_lstFileInfo.begin();
            while(fileite!= m_lstFileInfo.end()){
                if((*fileite)->fileId == atoll(FileId.c_str()) && sur->UserId == (*fileite)->userId){
                    //断点续传
                    surs.m_pos = (*fileite)->pos;
                    surs.m_Result = _uploadfileinfo_continue;
                    break;
                }
                ite++;
            }
        }
    }
    if(lsStr.size() > 0 && flag == false){
        surs.m_Result = _uploadfileinfo_flashtrans;
        //2、别人传的基于引用计数实现的   秒传  计数+1
        //sprintf(szsql,"update file set f_count = %lld where f_id = %lld");
        sprintf(szsql,"update file set f_count = f_count + 1 where f_id = %lld",atoll(FileId.c_str()));
        m_pSql->UpdateMysql(szsql);

        //修改映射
        sprintf(szsql,"insert into user_file(u_id,f_id,time) values(%lld,%lld,'%s')",
                sur->UserId,atoll(FileId.c_str()),sur->szFileUploadTime);
        m_pSql->UpdateMysql(szsql);
    }

    if(lsStr.size() == 0){
        surs.m_Result = _uploadfileinfo_normal;
        //3、如果文件不存在  正常传输  记录文件信息到数据库，映射表做映射
        //记录至数据库中
        char szFilePath[FILE_PATH] = {0};
        sprintf(szFilePath,"%s%lld/%s",m_szSystemPath,sur->UserId,sur->szFileName);
        sprintf(szsql,"insert into file(f_name,f_uploadtime,f_path,f_md5,f_size) values('%s','%s','%s','%s',%lld)",
                sur->szFileName,sur->szFileUploadTime,szFilePath,sur->szFileMD5,sur->szFileSize);
        m_pSql->UpdateMysql(szsql);
        //查找f_id
        sprintf(szsql,"select f_id from file where f_md5 = '%s'",sur->szFileMD5);
        m_pSql->SelectMysql(szsql,1,lsStr);
        if(lsStr.size()>0){
            string FileId = lsStr.front();
            lsStr.pop_front();
            surs.fileid = atoll(FileId.c_str());
            //修改映射
            sprintf(szsql,"insert into user_file(u_id,f_id,time) values(%lld,%lld,'%s')",sur->UserId,atoll(FileId.c_str()),sur->szFileUploadTime);
            m_pSql->UpdateMysql(szsql);
        }
        FILE* pFile = fopen(szFilePath,"ab");
        //记录文件指针，文件ID
        fileinfo *p = new fileinfo;
        p->userId = sur->UserId;
        p->fileId = surs.fileid;
        p->fileSize = sur->szFileSize;
        p->pos = 0;
        p->pFile = pFile;

        m_lstFileInfo.push_back(p);
    }
    //4、发送回复
    m_pNet->SendData(socketWaiter,(char*)&surs,sizeof(surs));
}

void Kernel::UploadFileContentRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_UPLOADFILECONTENT_RQ* psus = (STRU_UPLOADFILECONTENT_RQ*) szbuf;
    //接受到的 某一个文件的内容   怎么能找到这个文件？
    fileinfo* p = NULL;
    auto ite = m_lstFileInfo.begin();
    while(ite!= m_lstFileInfo.end()){
        if((*ite)->fileId == psus->fileid && (*ite)->userId == psus->userid){
            //找到当前文件内容的归属
            p = *ite;
            break;
        }
        ite++;
    }
    //根据文件指针写入内容
    size_t WriteNum = fwrite(psus->m_FileContent,sizeof(char),psus->m_fileNum,p->pFile);
    if(WriteNum > 0){
        p->pos += WriteNum;
        if(p->pos == p->fileSize){
            //当前文件的内容书写完毕
            fclose(p->pFile);
            delete p;
            m_lstFileInfo.erase(ite);
        }
    }
}

void Kernel::SelectFileRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_SELECTFILE_RQ* pssr = (STRU_SELECTFILE_RQ*)szbuf;
    STRU_SELECTFILE_RS sss;
    char szsql[SQLLEN] = {0};
    int index = 0;
    list<string> lsStr;
    //看是哪个人获取文件信息
    sprintf(szsql,"select f_name,f_size,f_uploadtime from ufile where u_id = %lld and f_name like '%%%s%%';",pssr->userid,pssr->m_KeyWord);
    //数据查询此人上传过的文件
    m_pSql->SelectMysql(szsql,3,lsStr);
    //封装进GetFileListRS结构体中
    while(lsStr.size()>0){
        string fileName = lsStr.front();
        lsStr.pop_front();

        string fileSize = lsStr.front();
        lsStr.pop_front();

        string fileTime = lsStr.front();
        lsStr.pop_front();

        //放入文件信息数组的哪个？
        strcpy(sss.arrFileInfo[index].szFileName,fileName.c_str());
        strcpy(sss.arrFileInfo[index].szFileUploadTime,fileTime.c_str());
        sss.arrFileInfo[index].szFileSize = atoll(fileSize.c_str());
        index++;
        if(index == FILE_NUM || lsStr.size() == 0){    //文件信息数组中已经包含15个文件   链表所有数据已经取出来了
            sss.szFileNume = index;
            //返回给客户端
            m_pNet->SendData(socketWaiter,(char*)&sss,sizeof(sss));
            ZeroMemory(sss.arrFileInfo,sizeof(sss.arrFileInfo)); //内存清空
            index = 0;
        }
    }
}

void Kernel::DeleteFileRq(SOCKET socketWaiter, const char *szbuf)
{
    //从数据库找到文件信息    --文件引用基计数 >1 修改引用计数的值 =1 删除文件信息删除文件本身
    //删除文件信息？
    STRU_DELETEFILE_RQ* psdr = (STRU_DELETEFILE_RQ*)szbuf;
    char szsql[SQLLEN] = {0};
    sprintf(szsql,"select f_id,f_count,f_path from ufile where u_id = %lld and f_name = '%s';",psdr->userId,psdr->szFileName);
    list<string> lsStr;
    m_pSql->SelectMysql(szsql,3,lsStr);
    if(lsStr.size() > 0){
        string f_id = lsStr.front();
        lsStr.pop_front();

        string f_count = lsStr.front();
        lsStr.pop_front();

        string f_path = lsStr.front();
        lsStr.pop_front();

        long long fcount = atoll(f_count.c_str());
        long long fid = atoll(f_id.c_str());
        if(fcount > 1){
            //修改引用计数
            sprintf(szsql,"update file set f_count = f_count - 1 where f_id =%lld",fid);
            m_pSql->UpdateMysql(szsql);
        }else{
            //删除文件信息
            sprintf(szsql,"delete from file where f_id = %lld",fid);
            m_pSql->UpdateMysql(szsql);
            //删除文件内容
            DeleteFileA(f_path.c_str());
        }
        //删除映射信息
        sprintf(szsql,"delete from user_file where u_id = %lld and f_id =%lld",psdr->userId,fid);
        m_pSql->UpdateMysql(szsql);
    }
}

void Kernel::SharedLinkRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_SHARELINK_RQ* pssr = (STRU_SHARELINK_RQ*)szbuf;
    STRU_SHARELINK_RS sss;
    char szsql[SQLLEN] = {0};
    list<string> lsStr;
    strcpy(sss.szFileName,pssr->szFileName);
    srand(time(NULL));
    //先生成四位随机数码
    char szCode[MAX_SIZE];
    char c;
    for(int i=0;i<4;i++){
        int num = rand()%36;    //0-9 数字    10-36 字母
        if(num<10)
            c = num +'0';
        else
            c = num - 10 +'A';
        szCode[i] = c;
    }
    strcpy(sss.szCode,szCode);
    //user_shared表中插入数据
    sprintf(szsql,"select f_id from ufile where u_id = %lld and f_name = '%s';",pssr->userId,pssr->szFileName);
    m_pSql->SelectMysql(szsql,1,lsStr);
    if(lsStr.size() > 0){
        string fileId = lsStr.front();
        lsStr.pop_front();

        sprintf(szsql,"insert into user_shared(uid,fid,code) values(%lld,%lld,'%s');",pssr->userId,atoll(fileId.c_str()),szCode);
        if(m_pSql->UpdateMysql(szsql)){
            strcpy(sss.szCode,szCode);
        }else{
            sprintf(szsql,"select code from user_shared where uid = %lld and fid = %lld; ",pssr->userId,atoll(fileId.c_str()));
            m_pSql->SelectMysql(szsql,1,lsStr);
            string code = lsStr.front();
            lsStr.pop_front();
            strcpy(sss.szCode,code.c_str());
        }
    }
    //返还给客户端一个回复
    m_pNet->SendData(socketWaiter,(char*)&sss,sizeof(sss));
}

void Kernel::GetLinkRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_GETLINK_RQ* psgr = (STRU_GETLINK_RQ*)szbuf;
    STRU_GETLINK_RS sgs;
    //code 码值查询数据
    char szsql[SQLLEN] = {0};
    list<string> lsStr;
    sprintf(szsql,"select uid,fid from user_shared where code = '%s';",psgr->szCode);
    m_pSql->SelectMysql(szsql,2,lsStr);
    if(lsStr.size()>0){
        //判断uid
        string Uid = lsStr.front();
        lsStr.pop_front();

        string Fid = lsStr.front();
        lsStr.pop_front();

        if(atoll(Uid.c_str()) == psgr->userId){
            sgs.szResult = _getlink_failed;
        }else{
            sgs.szResult = _getlink_success;
            //查询文件信息
            sprintf(szsql,"select f_name,f_size from ufile where f_id = %lld;",atoll(Fid.c_str()));
            m_pSql->SelectMysql(szsql,2,lsStr);
            if(lsStr.size()>0){
                strcpy(sgs.szFileName,lsStr.front().c_str());
                lsStr.pop_front();

                sgs.szFileSize = atoll(lsStr.front().c_str());
                lsStr.pop_front();

                strcpy(sgs.szFileUploadTime,psgr->szFileUploadTime);
                // lsStr.pop_front();
            }

            //修改文件的引用计数，向用户与文件映射表中插入一条数据
            sprintf(szsql,"insert into user_file(f_id,u_id,time) values(%lld,%lld,'%s');",atoll(Fid.c_str()),psgr->userId,psgr->szFileUploadTime);
            if(m_pSql->UpdateMysql(szsql)){
                sprintf(szsql,"update file set f_count = f_count + 1 where f_id = %lld",atoll(Fid.c_str()));
                m_pSql->UpdateMysql(szsql);
            }
        }
    }
    //返回给客户端
    m_pNet->SendData(socketWaiter,(char*)&sgs,sizeof(sgs));
}

void Kernel::DownloadFileInfoRq(SOCKET socketWaiter, const char *szbuf)
{
    STRU_DOWNLOADFILE_RQ* psdr = (STRU_DOWNLOADFILE_RQ*)szbuf;
    STRU_DOWNLOADFILE_RS sdrs;
    list<string> lsStr;
    //根据psdr中的文件名与用户id去视图中查找指定文件的存储路径
    char szsql[SQLLEN] = {0};
    sprintf(szsql,"select f_path from ufile where u_id = %lld and f_name = '%s';",psdr->userId,psdr->szFileName);
    m_pSql->SelectMysql(szsql,1,lsStr);
    if(lsStr.size() > 0){
        //向客户端返回文件内容
        string filePath = lsStr.front();
        lsStr.pop_front();

        FILE* pFile = fopen(filePath.c_str(),"rb");
        int num;
        while((num = fread(sdrs.m_FileContent,1,ONE_PAGE,pFile))>0){
            sdrs.m_fileNum = num;
            m_pNet->SendData(socketWaiter,(char*)&sdrs,sizeof(sdrs));
        }
        fclose(pFile);
    }
}













































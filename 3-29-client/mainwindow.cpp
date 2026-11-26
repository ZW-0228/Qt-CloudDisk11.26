#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pKernel = new Kernel;
    if(m_pKernel->Connect()){
        qDebug()<<"open success";
    }else{
        QMessageBox::critical(this,"result","connect err");   //错误对话框
    }
    m_pLogin = new login(m_pKernel);
    m_pLogin->setWindowTitle("登录");
    m_pLogin->setWindowIcon(QIcon(":/icon.png"));
    m_pLogin->show();
    m_pos = 0;
    connect((Kernel*)m_pKernel,&Kernel::loginRs,this,&MainWindow::loginRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::RegisterRs,m_pLogin,&login::RegisterRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::GetFileListRs,this,&MainWindow::GetFileListRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::UploadFileInfoRs,this,&MainWindow::UploadFileInfoRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::SelectFileRs,this,&MainWindow::SelectFileRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::SharedLinkRs,this,&MainWindow::SharedLinkRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::GetLinkRs,this,&MainWindow::GetLinkRs,Qt::BlockingQueuedConnection);
    connect((Kernel*)m_pKernel,&Kernel::DownLoadFileRs,this,&MainWindow::DownLoadFileRs,Qt::BlockingQueuedConnection);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pKernel;
    delete m_pLogin;
}

void MainWindow::showEvent(QShowEvent *event)
{
    ui->tableWidget->setColumnCount(3);
    //给表格设置表头
    QStringList lst;
    lst<<"文件名"<<"文件大小"<<"上传时间";
    ui->tableWidget->setHorizontalHeaderLabels(lst);   //设置表头文字
    //列平铺表格
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setStyleSheet("background-color:transparent;");
}

string FileDigest(const string &file) {     //根据文件内容和文件名生成文件的 MD5

    ifstream in(file.c_str(), std::ios::binary);
    if (!in)
        return "";

    MD5 md5;
    std::streamsize length;
    char buffer[1024];
    while (!in.eof()) {
        in.read(buffer, 1024);
        length = in.gcount();
        if (length > 0)
            md5.update(buffer, length);
    }
    in.close();
    return md5.toString();
}

void MainWindow::loginRs(const char *szbuf)
{
    STRU_LOGIN_RS* sls = (STRU_LOGIN_RS*)szbuf;
    if(sls->szResult == _login_res_failed){
        QMessageBox::warning(this,"登录结果","用户名或密码错误");
    }else if(sls->szResult == _login_res_noexist){
        QMessageBox::information(this,"登陆结果","用户不存在请注册");
    }else{
        //登陆成功
        //登录窗口隐藏
        m_pLogin->hide();
        //mainwindow显示
        this->show();
        //记录UserId，方便后续的获取文件列表操作
        Id=sls->szUserId;
        STRU_GETFILELIST_RQ sgr;
        sgr.szUserId = Id;
        m_pKernel->SendData((char*)&sgr,sizeof(sgr));
    }
}

void MainWindow::GetFileListRs(const char *szbuf)
{
    STRU_GETFILELIST_RS* psgr = (STRU_GETFILELIST_RS*)szbuf;
    for(int i=0;i<psgr->szFileNume;i++){
        int nRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(nRow);
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(QIcon(":/icon1.png"),psgr->arrFileInfo[i].szFileName));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(QString::number(psgr->arrFileInfo[i].szFileSize)));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(psgr->arrFileInfo[i].szFileUploadTime));
    }
}

void MainWindow::UploadFileInfoRs(const char *szbuf)
{
    STRU_UPLOADFILEINFO_RS* sur = (STRU_UPLOADFILEINFO_RS*)szbuf;
    //遍历链表获取当前回复所对应的文件信息
    uploadFileInfo* pInfo = NULL;
    auto ite = m_lstUploadFileInfo.begin();
    while(ite != m_lstUploadFileInfo.end()){
        if(strcmp(sur->szFileMD5,(*ite)->szFileMD5) == 0){
            //找到此文件，并记录
            pInfo = *ite;
            break;
        }
        ite++;
    }
    //上传文件信息的回复进行下一步操作

    switch(sur->m_Result){
    case _uploadfileinfo_repeat:
        QMessageBox::warning(this,"提示","您已上传过此文件，请不要重复上传");
            break;
    case _uploadfileinfo_flashtrans:
    {    QMessageBox::information(this,"提示","上传成功");
        //还得将新上传的文件显示在表格中   上传的文件   上传文件Action产生的
        //显示pInfo中的文件信息
        int nRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(nRow);

        QString strfileName = QString::fromStdString(string(pInfo->szFilePath)).section('/',-1);
        ui->tableWidget->setItem(nRow,0,new QTableWidgetItem(QIcon(":/icon1.png"),strfileName));
        ui->tableWidget->setItem(nRow,1,new QTableWidgetItem(QString::number(pInfo->m_szFileSize)));
        ui->tableWidget->setItem(nRow,2,new QTableWidgetItem(pInfo->szFileUploadTime));
        break;
    }
    case _uploadfileinfo_continue:
        //断点续传
    case _uploadfileinfo_normal:
        //需要发送文件内容
        //打开文件
        STRU_UPLOADFILECONTENT_RQ scr;
        scr.userid = Id;
        scr.fileid = sur->fileid;
        FILE *pFile = fopen(pInfo->szFilePath,"rb");    //b代表二进制文件
        //如果是断点续传，进行文件的偏移
        if(sur->m_Result == _uploadfileinfo_continue){
            fseek(pFile,sur->m_pos,SEEK_SET);
        }
        //读文件内容并发送
        while(1){
            size_t readNum = fread(scr.m_FileContent,sizeof(char),sizeof(scr.m_FileContent),pFile);
            if(readNum > 0){
                scr.m_fileNum = readNum;
                m_pKernel->SendData((char*)&scr,sizeof(scr));
            }else{
                break;
            }
        }
        //关闭文件
        fclose(pFile);
        //将文件信息显示在窗口
        break;
    }
    if(sur->m_Result != _uploadfileinfo_repeat){
        int nRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(nRow);

        QString strfileName = QString::fromStdString(string(pInfo->szFilePath)).section('/',-1);
        ui->tableWidget->setItem(nRow,0,new QTableWidgetItem(QIcon(":/icon1.png"),strfileName));
        ui->tableWidget->setItem(nRow,1,new QTableWidgetItem(QString::number(pInfo->m_szFileSize)));
        ui->tableWidget->setItem(nRow,2,new QTableWidgetItem(pInfo->szFileUploadTime));
    }
    delete pInfo;
    m_lstUploadFileInfo.erase(ite);
}

void MainWindow::SelectFileRs(const char *szbuf)    //服务器查到的15条文件，或者最后一次小于15条的
{   //第二次执行，显示16-30条文件
    STRU_SELECTFILE_RS* pssr = (STRU_SELECTFILE_RS*)szbuf;
    for(int i=0;i<pssr->szFileNume;i++){
        int nRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(nRow);
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(QIcon(":/icon1.png"),pssr->arrFileInfo[i].szFileName));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(QString::number(pssr->arrFileInfo[i].szFileSize)));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(pssr->arrFileInfo[i].szFileUploadTime));
    }
}

void MainWindow::SharedLinkRs(const char *szbuf)
{
    STRU_SHARELINK_RS* psss = (STRU_SHARELINK_RS*)szbuf;
    QString str = QString("分享文件:").append(psss->szFileName).append("成功，提取码是：").append(psss->szCode);
    QMessageBox::information(this,"分享文件",str);
}

void MainWindow::GetLinkRs(const char *szbuf)
{
    STRU_GETLINK_RS* psgr = (STRU_GETLINK_RS*)szbuf;
    //判断服务器处理的结果
    if(psgr->szResult == _getlink_failed){
        //失败  弹出提示框
        QMessageBox::warning(this,"结果","此文件您已拥有请不要重复提取");
    }else{

    int nRow = ui->tableWidget->rowCount();
    //表头 设置
    //成功    将文件信息追加至表格中
    ui->tableWidget->insertRow(nRow);

    ui->tableWidget->setItem(nRow,0,new QTableWidgetItem(QIcon(":/icon1.png"),psgr->szFileName));
    ui->tableWidget->setItem(nRow,1,new QTableWidgetItem(QString::number(psgr->szFileSize)));
    ui->tableWidget->setItem(nRow,2,new QTableWidgetItem(psgr->szFileUploadTime));
    }
}

void MainWindow::DownLoadFileRs(const char *szbuf)
{
    STRU_DOWNLOADFILE_RS* psdr = (STRU_DOWNLOADFILE_RS*)szbuf;
    //保存的文件路径
    FILE* pFile = fopen(filePath.c_str(),"ab");
    //一次写完？
    fseek(pFile,m_pos,SEEK_SET);
    int WriteNum = fwrite(psdr->m_FileContent,1,psdr->m_fileNum,pFile);
    if(WriteNum > 0){
        m_pos += WriteNum;  //构造函数中记得初始化
    }
    fclose(pFile);
}

void MainWindow::on_action_triggered()
{
    //获取要上传的文件信息 --- 文件名、文件大小、文件上传时间、文件的唯一标识MD5值
    QString filePath = QFileDialog::getOpenFileName(this,tr("打开文件"),
                                                    ".",    //起始路径
                                                    tr("All Files (*.*);;Images (*.png *.xmp *.jpg);;Text files (*.txt);;XML files (*.xml)"));
    qDebug()<<filePath.section('/',-1);
    QString fileName = filePath.section('/',-1);
    if(fileName == "") return;
    //文件大小
    QFile qfile(filePath);
    qfile.open(QIODevice::ReadOnly);
    qint64 fileSize = qfile.size();
    qfile.close();

    //MD5
    string strMD5 = FileDigest(filePath.toStdString());
    //上传时间
    QDateTime time = QDateTime::currentDateTime();
    QString strTime = time.toString("yyyy-MM-dd HH:mm:ss");
    //qDebug()<<strTime;

    //发送上传文件信息请求 ----- 告诉服务器我要上传哪个文件
    STRU_UPLOADFILEINFO_RQ sur;
    strcpy_s(sur.szFileMD5,strMD5.c_str());
    strcpy_s(sur.szFileName,fileName.toStdString().c_str());
    strcpy_s(sur.szFileUploadTime,strTime.toStdString().c_str());
    sur.szFileSize = fileSize;
    sur.UserId = Id;

    //收到服务器准许上传文件内容的回复时怎么知道我要传输哪个文件?
    //记录文件信息　文件名、路径、文件ID、文件大小
    uploadFileInfo *pInfo = new uploadFileInfo;
    pInfo->m_szFileSize = fileSize;
    strcpy_s(pInfo->szFileUploadTime,strTime.toStdString().c_str());
    strcpy_s(pInfo->szFilePath,filePath.toStdString().c_str());
    strcpy_s(pInfo->szFileMD5,strMD5.c_str());
    pInfo->m_pos = 0;
    m_lstUploadFileInfo.push_back(pInfo);

    m_pKernel->SendData((char*)&sur,sizeof(sur));
}

void MainWindow::on_select_clicked()
{
    //获取文本框内的关键字    结合用户id
    STRU_SELECTFILE_RQ ssr;
    QString keyWord = ui->lineEdit->text();
    strcpy(ssr.m_KeyWord,keyWord.toStdString().c_str());
    ssr.userid = Id;
    m_pKernel->SendData((char*)&ssr,sizeof(ssr));
    ui->tableWidget->setRowCount(0);
}


void MainWindow::on_Delete_clicked()
{
    //用户点击表格内的一行，删除这个文件 用户根据文件名删除文件
    //获取文件的名字
    int nRow = ui->tableWidget->currentRow();
    if(nRow < 0)
        return ;
    QString fileName = ui->tableWidget->item(nRow,0)->text();

    //fileName Userid 发送给服务器
    STRU_DELETEFILE_RQ sdr;
    sdr.userId = Id;
    strcpy(sdr.szFileName,fileName.toStdString().c_str());
    if(m_pKernel->SendData((char*)&sdr,sizeof(sdr))){
        QMessageBox::information(this,"删除文件","此文件已成功删除");
        ui->tableWidget->removeRow(nRow);
    }
}

//修改，直接在表格内进行修改  ---点击修改按钮  --把表格中变动哪一行的数据发送给服务器 ---写入至数据库

void MainWindow::on_action_3_triggered()    //分享文件
{
    int nRow = ui->tableWidget->currentRow();
    if(nRow < 0)
        return ;
    QString fileName = ui->tableWidget->item(nRow,0)->text();
    STRU_SHARELINK_RQ ssr;
    ssr.userId = Id;
    strcpy(ssr.szFileName,fileName.toStdString().c_str());
    m_pKernel->SendData((char*)&ssr,sizeof(ssr));
}





void MainWindow::on_action_4_triggered()
{
    STRU_GETLINK_RQ sgr;
    //弹出对话框
    m_pDialog = new Dialog(this);
    int Result = m_pDialog->exec();
    //判断对话框点击的结果
    if(Result == QDialog::Accepted){
        QDateTime time = QDateTime::currentDateTime();
        QString strTime = time.toString("yyyy-MM-dd HH:mm:ss");
        strcpy(sgr.szCode,m_pDialog->m_code.c_str());
        sgr.userId = Id;
        strcpy(sgr.szFileUploadTime,strTime.toStdString().c_str());
    }
    m_pKernel->SendData((char*)&sgr,sizeof(sgr));
        //确定获取分享码
}


void MainWindow::on_action_2_triggered()
{
    STRU_DOWNLOADFILE_RQ sdr;
    //先获取文件信息
    int nRow = ui->tableWidget->currentRow();
    if(nRow == -1)
        return;
    filePath = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                "./test.png",
                                                tr("Images (*.png *.xmp *.jpg);;All Files (*.*);;Text files (*.txt);;XML files (*.xml)")).toStdString();
    //文件名字与用户id组合发送至服务器
    string fileName = ui->tableWidget->item(nRow,0)->text().toStdString();
    sdr.userId = Id;
    strcpy(sdr.szFileName,fileName.c_str());
    m_pKernel->SendData((char*)&sdr,sizeof(sdr));
}


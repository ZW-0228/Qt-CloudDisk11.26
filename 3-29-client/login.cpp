#include "login.h"
#include "ui_login.h"

login::login(IKernel* pKernel,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);
    m_pKernel = pKernel;
    //connect((Kernel*)m_pKernel,&Kernel::RegisterRs,this,&login::RegisterRs);
}

login::~login()
{
    delete ui;
}

void login::on_pushButton_clicked() //登录
{
    //采集页面上的信息
    QString UserName = ui->lineEdit->text();
    QString PassWord = ui->lineEdit_2->text();
    //封装成STRU_LOGIN_RQ的形式的信息
    STRU_LOGIN_RQ slr;
    strcpy(slr.szName,UserName.toStdString().c_str());
    strcpy(slr.szPassword,PassWord.toStdString().c_str());
    //发送至服务器
    m_pKernel->SendData((char*)&slr,sizeof(slr));
}


void login::on_pushButton_2_clicked()   //注册
{
    //创建register窗体      kernel传输
    m_Register = new Register(m_pKernel);
    //隐藏自己
    this->hide();
    //显示register
    m_Register->show();
}

void login::RegisterRs(const char *szbuf)
{
    STRU_REGISTER_RS* srr = (STRU_REGISTER_RS*)szbuf;
    if(srr->szResult == _register_res_failed){
        QMessageBox::critical(this,"提示","请输入正确的用户名和电话号：保持唯一属性");
    }else{
        QMessageBox::information(this,"提示","注册成功，请返回继续登录");
        m_Register->hide();
        this->show();
    }
}



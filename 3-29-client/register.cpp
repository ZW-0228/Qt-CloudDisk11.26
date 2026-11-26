#include "register.h"
#include "ui_register.h"

Register::Register(IKernel*pKernel,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Register)
{
    ui->setupUi(this);
    m_pKernel = pKernel;
    this->setWindowIcon(QIcon(":/icon.png"));
    this->setWindowTitle("注册");
}

Register::~Register()
{
    delete ui;
}

void Register::on_pushButton_3_clicked()
{
    //跟登录一个思路    没有拿到kernel
    //采集界面上的信息
    QString UserName = ui->lineEdit->text();
    QString PassWord = ui->lineEdit_2->text();
    QString Tel = ui->lineEdit_3->text();
    //格式校验
    bool A=false,b=false,Num=false;
    for(int i=0;i<PassWord.size();i++){
        if(PassWord[i]>='0' && PassWord[i]<='9'){
            //存在数字
            Num=true;
        }else if(PassWord[i]>='a' && PassWord[i]<='z'){
            //存在小写字母
            b=true;
        }else if(PassWord[i]>='A' && PassWord[i]<='Z'){
            //存在大写字母
            A=true;
        }
    }
    if(!(A&&b&&Num)){
        QMessageBox::warning(this,"","请输入合法的密码：包含大写字母A-Z，小写字母a-z，数字0-9");
        ui->lineEdit_2->setFocus();
        return;
    }

    //判断电话
    if(Tel.toLongLong()<10000000000 || Tel.toLongLong()>99999999999){
        QMessageBox::warning(this,"","请输入合法的电话号：电话号码为11位");
        ui->lineEdit_3->setFocus();
        return;
    }
    //封装信息
    STRU_REGISTER_RQ srr;
    strcpy(srr.szName,UserName.toStdString().c_str());
    strcpy(srr.szPassword,PassWord.toStdString().c_str());
    srr.szTel = Tel.toLongLong();
    //发送到服务器
    m_pKernel->SendData((char*)&srr,sizeof(srr));
}


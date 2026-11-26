#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QMessageBox>
#include "packdef.h"
#include "./Kernel/Kernel.h"
#include "register.h"
namespace Ui {
class login;
}

class login : public QWidget
{
    Q_OBJECT

public:
    explicit login(IKernel* pKernel,QWidget *parent = nullptr);
    ~login();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();
public slots:
    void RegisterRs(const char *szbuf);


private:
    Ui::login *ui;
    IKernel* m_pKernel;
    Register* m_Register;
};

#endif // LOGIN_H

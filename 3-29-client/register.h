#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>
#include <QMessageBox>
#include "./Kernel/Kernel.h"
namespace Ui {
class Register;
}

class Register : public QWidget
{
    Q_OBJECT

public:
    explicit Register(IKernel*pKernel,QWidget *parent = nullptr);
    ~Register();

private slots:
    void on_pushButton_3_clicked();

private:
    Ui::Register *ui;
    IKernel* m_pKernel;
};

#endif // REGISTER_H

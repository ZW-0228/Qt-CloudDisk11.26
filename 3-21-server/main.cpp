#include <QCoreApplication>
#include"./Kernel/kernel.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(Kernel::GetKernel()->open()){
        cout<<"Server is running"<<endl;
    }

    return a.exec();
}

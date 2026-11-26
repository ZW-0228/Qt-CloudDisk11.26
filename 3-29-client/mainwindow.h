#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "./Kernel/Kernel.h"
#include <QDebug>
#include <QMessageBox>
#include "login.h"     //""  <>直接用系统的
#include "packdef.h"
#include <QFileDialog>
#include "./MD5/md5.h"
#include <QDateTime>
#include <dialog.h>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
struct uploadFileInfo{
    char szFilePath[FILE_PATH];
    char szFileUploadTime[MAX_SIZE];
    char szFileMD5[MAX_SIZE];
    long long m_szFileSize;
    long long m_pos;    //偏移量
};
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showEvent(QShowEvent *event);
    string fileDigest(const string &file);
private slots:
    void loginRs(const char* szbuf);
    void GetFileListRs(const char* szbuf);
    void UploadFileInfoRs(const char* szbuf);
    void SelectFileRs(const char* szbuf);
    void SharedLinkRs(const char* szbuf);
    void GetLinkRs(const char* szbuf);
    void DownLoadFileRs(const char* szbuf);
    void on_action_triggered();
    void on_select_clicked();
    void on_Delete_clicked();
    void on_action_3_triggered();
    void on_action_4_triggered();
    void on_action_2_triggered();

private:
    Ui::MainWindow *ui;
    IKernel* m_pKernel;
    login* m_pLogin;
    long long Id;
    std::list<uploadFileInfo*> m_lstUploadFileInfo;
    Dialog* m_pDialog;
    string filePath;
    int m_pos;
};
#endif // MAINWINDOW_H

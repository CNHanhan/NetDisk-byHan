#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "./kernel/IKernel.h"
#include "pack.h"
#include "login.h"
#include <list>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

struct f_info
{
    char fileName[MAXSIZE];
    char filePath[MAX_PATH];
    char md5[MAXSIZE];
    long long pos;
    long long fileSize;
    char uploadtime[MAXSIZE];
};

struct f_down
{
    FILE* pf;
    long long pos;
    long long fileSize;
    long long fileId;
};


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    IKernel* m_pKernel;
    Login* m_pLogin;
    long long m_userId;
    std::list<f_info*> m_lstFileInfo;
    std::vector<long long> m_tableToFileID;
    std::list<f_down*> m_lstfileDown;

public slots:
    void slots_loginrs(STRU_LOGIN_RS*);
    void slots_filelistrs(STRU_GETFILELIST_RS*);
    void slots_uploadfileinfors(STRU_UPLOADFILEINFO_RS*);
    void slots_downloadfilers(STRU_DOWNLOADFILE_RS*);
    void slots_sharedfilers(STRU_SHAREDFILE_RS*);
    void slots_importfilers(STRU_IMPORTFILE_RS*);
private slots:
    void on_btn_uploadfile_clicked();
    void on_btn_downloadFile_clicked();
    void on_btn_sharedfile_clicked();
    void on_btn_importlink_clicked();
    void on_btn_deletefile_clicked();
};
#endif // WIDGET_H

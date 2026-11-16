#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <map>
#include "./kernel/IKernel.h"
#include "pack.h"
#include "login.h"
#include <list>
#include <vector>
#include <QLabel>
#include <QProgressBar>


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
    // ---- 下载进度显示（不保存 QProgressBar 指针，防止控件被销毁后变成野指针）----
    int row;                 // 该文件在表格中的行号
    qint64 lastUpdateMs;     // 上次刷新界面的时刻（用于算速度）
    long long lastUpdatePos; // 上次刷新时已下载字节数
    double speedKB;          // 平滑后的下载速度（KB/s）
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
    std::list<f_down*> m_lstfileDown;

    // 好友与聊天窗口组件
    QDialog* m_pChatDialog;
    QListWidget* m_pFriendListWidget;
    QTextBrowser* m_pChatHistoryView;
    QLineEdit* m_pChatInputEdit;
    std::map<long long, QString> m_mapFriends;       // friendId -> friendName
    std::map<long long, QString> m_mapChatHistory;   // friendId -> 聊天记录HTML
    void initChatDialog();
    QLabel* m_pUserNameLabel;
    // 把表格第 row 行的“文件状态”列设置为纯文本（不可编辑）
    void showStatusText(int row, const QString& text);
    // 取第 row 行对应的文件 id（存在第 0 列项的 UserRole 里，没取到返回 -1）
    long long fileIdAtRow(int row) const;


public slots:
    void slots_loginrs(STRU_LOGIN_RS*);
    void slots_filelistrs(STRU_GETFILELIST_RS*);
    void slots_uploadfileinfors(STRU_UPLOADFILEINFO_RS*);
    void slots_downloadfilers(STRU_DOWNLOADFILE_RS*);
    void slots_sharedfilers(STRU_SHAREDFILE_RS*);
    void slots_importfilers(STRU_IMPORTFILE_RS*);

    // 好友与聊天槽函数
    void slots_addfriendrs(STRU_ADDFRIEND_RS*);
    void slots_friendlistrs(STRU_GETFRIENDLIST_RS*);
    void slots_chatrq(STRU_CHAT_RQ*);
    void slots_chatrs(STRU_CHAT_RS*);

private slots:
    void on_btn_uploadfile_clicked();
    void on_btn_downloadFile_clicked();
    void on_btn_sharedfile_clicked();
    void on_btn_importlink_clicked();
    void on_btn_deletefile_clicked();
    void on_pushButton_6_clicked(); // 添加好友按钮
    void on_pushButton_7_clicked(); // 好友聊天按钮
    void slot_sendChatMsg();
};
#endif // WIDGET_H

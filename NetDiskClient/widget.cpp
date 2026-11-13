#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include "./kernel/tcpkernel.h"
#include "login.h"
#include <QIcon>
#include <QMessageBox>
#include <QFileDialog>
#include "./md5/md5.h"
#include <QDateTime>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_pKernel = new TCPKernel;
    if (!m_pKernel->connectServer("192.168.216.131"))
    {
        QMessageBox::critical(this,"错误","链接服务器失败！");
        return;
    }
    m_pLogin = new Login;
    m_pLogin->setKernel(m_pKernel);
    m_pLogin->show();
    setWindowIcon(QIcon(":/netdisk.png"));

    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_regrs, m_pLogin, &Login::slots_regrs,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_loginrs, this, &Widget::slots_loginrs,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_filelistrs, this, &Widget::slots_filelistrs,Qt::BlockingQueuedConnection);

    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_uploadfileinfors, this, &Widget::slots_uploadfileinfors,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_downloadfilers, this, &Widget::slots_downloadfilers,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_sharedfilers, this, &Widget::slots_sharedfilers,Qt::BlockingQueuedConnection);

    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_importfilers, this, &Widget::slots_importfilers,Qt::BlockingQueuedConnection);

}

Widget::~Widget()
{
    delete m_pKernel;
    delete ui;
}

void Widget::slots_loginrs(STRU_LOGIN_RS * pslr)
{
    if (pslr->m_ret == LOGIN_USER_ERR)
    {
        QMessageBox::critical(this,"错误","用户名不存在！");
        return;
    }
    else if(pslr->m_ret == LOGIN_PWD_ERR)
    {
        QMessageBox::critical(this,"错误","密码错误！");
        return;
    }
    // 登录成功
    m_pLogin->close();
    delete m_pLogin;
    // 显示主窗口
    this->show();
    m_userId = pslr->m_userId;

    // 请求当前用户的文件列表
    STRU_GETFILELIST_RQ sgr;
    sgr.m_userId = m_userId;
    m_pKernel->sendData((char*)&sgr, sizeof(sgr));


}

void Widget::slots_filelistrs(STRU_GETFILELIST_RS * psgr)
{
    // a  100     1 20   2 20   3 20
    // 获取当前行大小，重新设置 当前行大小+ 文件个数
    int count = ui->tableWidget->rowCount();            //      0           20      40
    ui->tableWidget->setRowCount(count + psgr->m_fileNum);  //  20          40      60
    for(int i = 0; i < psgr->m_fileNum; i++)
    {
        QString fileName = QString::fromUtf8(psgr->m_fileInfo[i].m_filename);
        QString fileSize = QString::number(psgr->m_fileInfo[i].m_filesize);
        QString uploadTime = QString::fromUtf8(psgr->m_fileInfo[i].m_uploadtime);
        QString finish = psgr->m_fileInfo[i].m_finish ? "上传成功":"上传未完成";
        ui->tableWidget->setItem(count, 0, new QTableWidgetItem(QIcon(":/file.png"), fileName));
        ui->tableWidget->setItem(count, 1, new QTableWidgetItem(fileSize));
        ui->tableWidget->setItem(count, 2, new QTableWidgetItem(uploadTime));
        ui->tableWidget->setItem(count, 3, new QTableWidgetItem(finish));
        m_tableToFileID.push_back(psgr->m_fileInfo[i].m_fileID);
        count++;
    }

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

// 如果当前客户端一次要上传多个文件-》需要使用线程池-》把要传输的文件放到链表记录，服务端发送回复后找到要传输的文件进行传输
void Widget::slots_uploadfileinfors(STRU_UPLOADFILEINFO_RS *psur)
{
    // 从链表遍历找到要传输的文件
    auto it = m_lstFileInfo.begin();
    f_info* p = nullptr;
    while(it != m_lstFileInfo.end())
    {
        if(!strcmp((*it)->md5, psur->m_md5))
        {
            p = *it;
            break;
        }
        it++;
    }
    STRU_UPLOADFILESTREAM_RQ sur;
    sur.m_fileID = psur->m_fileId;
    sur.m_userID = m_userId;
    if (!p) return;
    switch(psur->m_ret)
    {
    case UPLOAD_FILEINFO_CONTINUE:
    case UPLOAD_FILEINFO_CONTINUE | UPLOAD_FILEINFO_FALSH:
    case UPLOAD_FILEINFO_NORMAL:        // 正常传
        {
            // 构建一个发送包
            // 找到文件位置 fopen(文件路径， 只读并且二进制模式打开)
            // 打开文件
            FILE* pf = fopen(p->filePath, "rb");
            // 判断是否是断点续传
            if(psur->m_ret & UPLOAD_FILEINFO_CONTINUE)
            {
                fseek(pf, psur->m_pos, SEEK_SET);
            }
            // 循环读取文件并发送给服务器
            while(1)
            {
                int num = fread(sur.m_Stream, 1, ONEPAGE, pf);
                sur.m_StreamLen = num;
                // 关闭文件
                if (num <=0 )
                {
                    fclose(pf);
                    break;
                }
                // 将数据包发送给服务器
                m_pKernel->sendData((char*)&sur, sizeof(sur));
            }
            // 将文件信息显示到界面列表上
            if(psur->m_ret & UPLOAD_FILEINFO_NORMAL || psur->m_ret & UPLOAD_FILEINFO_FALSH)
            {
                STRU_GETFILELIST_RS sgr;
                sgr.m_fileNum = 1;
                strcpy(sgr.m_fileInfo[0].m_filename, p->fileName);
                strcpy(sgr.m_fileInfo[0].m_uploadtime, p->uploadtime);
                sgr.m_fileInfo[0].m_filesize = p->fileSize;
                sgr.m_fileInfo[0].m_finish = 1;
                sgr.m_fileInfo[0].m_fileID = psur->m_fileId;
                slots_filelistrs(&sgr);
            }
        }
        break;
    case UPLOAD_FILEINFO_FALSH:
    {
        STRU_GETFILELIST_RS sgr;
        sgr.m_fileNum = 1;
        strcpy(sgr.m_fileInfo[0].m_filename, p->fileName);
        strcpy(sgr.m_fileInfo[0].m_uploadtime, p->uploadtime);
        sgr.m_fileInfo[0].m_filesize = p->fileSize;
        sgr.m_fileInfo[0].m_finish = 1;
        sgr.m_fileInfo[0].m_fileID = psur->m_fileId;
        slots_filelistrs(&sgr);
    }
        break;
    case UPLOAD_FILEINFO_REPEAT:
        QMessageBox::critical(this, "错误", "重复上传文件！");
        break;
    }
    delete p;
    m_lstFileInfo.erase(it);
}

void Widget::slots_downloadfilers(STRU_DOWNLOADFILE_RS * psdr)
{
    // 下载文件的处理
    auto it = m_lstfileDown.begin();
    f_down* p = nullptr;
    while(it != m_lstfileDown.end())
    {
        if((*it)->fileId == psdr->fileId)
        {
            p = *it;
            break;
        }
        it++;
    }
    if(!p) return;
    if(psdr->m_ret == DOWNLOADFILE_ERR)
    {
        // 不能下载
        QMessageBox::critical(this, "错误","不允许下载该文件");
        fclose(p->pf);
        delete p;
        m_lstfileDown.erase(it);
        return;
    }

    p->pos +=  fwrite(psdr->fileStream, 1, psdr->fileStreamLen, p->pf);
    if(p->pos == p->fileSize)
    {
        QMessageBox::information(this,"提示","下载成功");
        fclose(p->pf);
        delete p;
        m_lstfileDown.erase(it);
    }

}

void Widget::slots_sharedfilers(STRU_SHAREDFILE_RS *pssr)
{
    if(pssr->m_ret == SHARED_ERR)
    {
        QMessageBox::critical(this,"错误","分享失败");
        return;
    }
    QMessageBox::information(this,"提示","分享成功，分享码在编辑框中");
    ui->lineEdit->setText(pssr->m_code);
}

void Widget::slots_importfilers(STRU_IMPORTFILE_RS *psir)
{
    if(psir->m_ret == IMPORT_ERR)
    {
        QMessageBox::critical(this,"错误","提取失败");
        return;
    }
    STRU_GETFILELIST_RS sgr;
    sgr.m_fileNum = 1;
    sgr.m_fileInfo[0].m_fileID = psir->info.m_fileID;
    sgr.m_fileInfo[0].m_filesize = psir->info.m_filesize;
    strcpy(sgr.m_fileInfo[0].m_filename, psir->info.m_filename);
    strcpy(sgr.m_fileInfo[0].m_uploadtime, psir->info.m_uploadtime);
    sgr.m_fileInfo[0].m_finish = psir->info.m_finish;
    slots_filelistrs(&sgr);

}

void Widget::on_btn_uploadfile_clicked()
{
    STRU_UPLOADFILEINFO_RQ sur;
    // 打开一个窗口选择文件
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"),
                                                    "./",
                                                    tr("ALL FILE (*.*);;Images (*.png *.xpm *.jpg);;Text files (*.txt)"));
    if(!strcmp("", filePath.toStdString().c_str())) return;
    QString fileName = filePath.section('/', -1);
    // 获取文件名字
    strcpy(sur.m_fileinfo.m_filename, fileName.toStdString().c_str());
    long long fileSize = 0;
    // 获取文件大小
    {
        QFile qFile(filePath);
        qFile.open(QIODevice::ReadOnly);
        fileSize = qFile.size();
        qFile.close();
    }
    sur.m_fileinfo.m_filesize = fileSize;
    // 获取文件MD5值
    std::string md5 = MD5::getFileMD5(filePath.toStdString());
    strcpy(sur.m_md5, md5.c_str());
    sur.m_userId = m_userId;
    // 获取当前时间
    strcpy(sur.m_fileinfo.m_uploadtime, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str());
    // 发送
    m_pKernel->sendData((char*)&sur, sizeof(sur));
    f_info* p = new f_info;
    strcpy(p->fileName, fileName.toStdString().c_str());
    strcpy(p->filePath, filePath.toStdString().c_str());
    strcpy(p->md5, md5.c_str());
    strcpy(p->uploadtime, sur.m_fileinfo.m_uploadtime);
    p->pos = 0;
    p->fileSize = fileSize;
    m_lstFileInfo.push_back(p);

}


void Widget::on_btn_downloadFile_clicked()
{
    // 下载文件
    // 检查是否选中文件
    int index = ui->tableWidget->currentRow();
    if(index == -1)
    {
        QMessageBox::critical(this,"错误","请选择要下载的文件");
        return;
    }
    STRU_DOWNLOADFILE_RQ sdr;
    sdr.userId = m_userId;
    sdr.fileId = m_tableToFileID[index];
    // 获取要保存的文件名
    char fileBase[MAX_PATH] = {0};
    char fileName[MAXSIZE] = {0};
    strcpy(fileName, ui->tableWidget->item(index,0)->text().toStdString().c_str());
    sprintf(fileBase,"./%s",fileName);
    // 先选择要保存的文件位置
    QString filePath = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                                    fileBase,
                                                    tr("ALL FILE (*)"));

    if(!strcmp("", filePath.toStdString().c_str())) return;

    FILE* pf = fopen(filePath.toStdString().c_str(), "wb");
    f_down* p = new f_down;
    p->fileId = sdr.fileId;
    p->fileSize = atoll( ui->tableWidget->item(index,1)->text().toStdString().c_str());
    p->pf = pf;
    p->pos = 0;
    m_lstfileDown.push_back(p);
    m_pKernel->sendData((char*)&sdr,sizeof(sdr));
}


void Widget::on_btn_sharedfile_clicked()
{
    // 分享选中的文件
    int index = ui->tableWidget->currentRow();
    if(index == -1)
    {
        QMessageBox::critical(this,"错误","请选择要分享的文件");
        return;
    }
    long long fileId = m_tableToFileID[index];
    STRU_SHAREDFILE_RQ ssr;
    ssr.fileId = fileId;
    ssr.userId = m_userId;
    strcpy(ssr.sharedtime,QDateTime::currentDateTime().addDays(7).toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str());
    m_pKernel->sendData((char*)&ssr,sizeof(ssr));
}


void Widget::on_btn_importlink_clicked()
{
    // 验证编辑框中文本长度是否和提取码长度相同
    int num = ui->lineEdit->text().length();
    if(num != CODELEN - 1)
    {
        // 不同 -》错误提示，重新填写提取码
        QMessageBox::critical(this,"错误","提取码长度有误");
        return;
    }
    // 相同 构建包-》发送
    STRU_IMPORTFILE_RQ sir;
    sir.userId = m_userId;
    strcpy(sir.code, ui->lineEdit->text().toStdString().c_str());
    m_pKernel->sendData((char*)&sir, sizeof(sir));

}



void Widget::on_btn_deletefile_clicked()
{
    // 删除选中的文件
    int index = ui->tableWidget->currentRow();
    if(index == -1)
    {
        QMessageBox::critical(this,"错误","请选择要删除的文件");
        return;
    }
    long long fileId = m_tableToFileID[index];
    STRU_DELETEFILE_RQ ssr;
    ssr.fileId = fileId;
    ssr.userId = m_userId;
    ui->tableWidget->removeRow(index);
    m_tableToFileID.erase(m_tableToFileID.begin() + index);
    m_pKernel->sendData((char*)&ssr,sizeof(ssr));
}


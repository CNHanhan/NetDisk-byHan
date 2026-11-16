#include "widget.h"
#include "ui_widget.h"
#include "config.h"
#include <QDebug>
#include "./kernel/tcpkernel.h"
#include "login.h"
#include <QIcon>
#include <QMessageBox>
#include <QFileDialog>
#include "./md5/md5.h"
#include <QDateTime>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QProgressBar>
#include <deque>



Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_pKernel = new TCPKernel;
    if (!m_pKernel->connectServer(SERVER_IP, SERVER_PORT))
    {
        QMessageBox::critical(this,"错误","链接服务器失败！");
        return;
    }
    m_pLogin = new Login;
    m_pLogin->setKernel(m_pKernel);
    m_pLogin->show();
    // 当主窗口还没显示、用户直接关闭登录框时，让整个程序自动退出
    this->setAttribute(Qt::WA_QuitOnClose, true);
    m_pLogin->setAttribute(Qt::WA_QuitOnClose, true);

    setWindowIcon(QIcon(":/netdisk.png"));
    // 在主窗口左上角（左侧按钮栏最上方）添加头像和用户名
    QWidget* userHeaderWidget = new QWidget(this);
    QHBoxLayout* userHeaderLayout = new QHBoxLayout(userHeaderWidget);
    userHeaderLayout->setContentsMargins(4, 8, 4, 12);

    QLabel* avatarLabel = new QLabel(userHeaderWidget);
    avatarLabel->setPixmap(QPixmap(":/netdisk.png").scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    avatarLabel->setFixedSize(46, 46);
    avatarLabel->setStyleSheet("border: 2px solid #3498db; border-radius: 23px; padding: 2px; background: white;");

    m_pUserNameLabel = new QLabel("未登录", userHeaderWidget);
    m_pUserNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");

    userHeaderLayout->addWidget(avatarLabel);
    userHeaderLayout->addWidget(m_pUserNameLabel);
    userHeaderLayout->addStretch();

    // 插入到左侧 verticalLayout 的第 0 个位置（所有按钮的最上方）
    ui->verticalLayout->insertWidget(0, userHeaderWidget);


    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_regrs, m_pLogin, &Login::slots_regrs,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_loginrs, this, &Widget::slots_loginrs,Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel,&TCPKernel::signal_filelistrs, this, &Widget::slots_filelistrs,Qt::BlockingQueuedConnection);
    initChatDialog();
    connect((TCPKernel*)m_pKernel, &TCPKernel::signal_addfriendrs, this, &Widget::slots_addfriendrs, Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel, &TCPKernel::signal_friendlistrs, this, &Widget::slots_friendlistrs, Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel, &TCPKernel::signal_chatrq, this, &Widget::slots_chatrq, Qt::BlockingQueuedConnection);
    connect((TCPKernel*)m_pKernel, &TCPKernel::signal_chatrs, this, &Widget::slots_chatrs, Qt::BlockingQueuedConnection);


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

    // 登录成功，获取刚才输入的用户名显示到主窗口左上角和窗口标题
    QString currentUserName = m_pLogin->getLoginUserName();
    m_pUserNameLabel->setText(currentUserName);
    this->setWindowTitle(QString("个人云网盘 - 当前用户：%1").arg(currentUserName));

    m_pLogin->setAttribute(Qt::WA_QuitOnClose, false);
    m_pLogin->close();

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
        QTableWidgetItem* itemName   = new QTableWidgetItem(QIcon(":/file.png"), fileName);
        QTableWidgetItem* itemSize   = new QTableWidgetItem(fileSize);
        QTableWidgetItem* itemTime   = new QTableWidgetItem(uploadTime);
        QTableWidgetItem* itemFinish = new QTableWidgetItem(finish);

        // 把文件 id 存在第 0 列项里，后面下载/分享/删除都靠它定位
        itemName->setData(Qt::UserRole, QVariant::fromValue(psgr->m_fileInfo[i].m_fileID));

        // 除去文件名（itemName 保持可编辑），其他三列全部设为不可修改
        itemSize->setFlags(itemSize->flags() & ~Qt::ItemIsEditable);
        itemTime->setFlags(itemTime->flags() & ~Qt::ItemIsEditable);
        itemFinish->setFlags(itemFinish->flags() & ~Qt::ItemIsEditable);

        ui->tableWidget->setItem(count, 0, itemName);
        ui->tableWidget->setItem(count, 1, itemSize);
        ui->tableWidget->setItem(count, 2, itemTime);
        ui->tableWidget->setItem(count, 3, itemFinish);

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
    case UPLOAD_FILEINFO_NORMAL:        // 正常传 / 断点续传
    {
        FILE* pf = fopen(p->filePath, "rb");
        if (!pf) break;

        long long sentBytes = 0;
        if(psur->m_ret & UPLOAD_FILEINFO_CONTINUE)
        {
            sentBytes = psur->m_pos;
            fseek(pf, psur->m_pos, SEEK_SET);
        }

        // 1. 先在表格中定位或新增当前上传文件的行（fileId 从第 0 列的 UserRole 里取）
        int row = -1;
        for (int i = 0; i < ui->tableWidget->rowCount(); i++)
        {
            if (fileIdAtRow(i) == psur->m_fileId)
            {
                row = i;
                break;
            }
        }
        if (row == -1)
        {
            row = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(row + 1);
            QTableWidgetItem* itemName = new QTableWidgetItem(QIcon(":/file.png"), QString::fromUtf8(p->fileName));
            QTableWidgetItem* itemSize = new QTableWidgetItem(QString::number(p->fileSize));
            QTableWidgetItem* itemTime = new QTableWidgetItem(QString::fromUtf8(p->uploadtime));
            QTableWidgetItem* itemStat = new QTableWidgetItem("");

            itemName->setData(Qt::UserRole, QVariant::fromValue(psur->m_fileId));
            itemSize->setFlags(itemSize->flags() & ~Qt::ItemIsEditable);
            itemTime->setFlags(itemTime->flags() & ~Qt::ItemIsEditable);
            itemStat->setFlags(itemStat->flags() & ~Qt::ItemIsEditable);

            ui->tableWidget->setItem(row, 0, itemName);
            ui->tableWidget->setItem(row, 1, itemSize);
            ui->tableWidget->setItem(row, 2, itemTime);
            ui->tableWidget->setItem(row, 3, itemStat);

            ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        }

        // 2. 在第 3 列（文件状态）放入 QProgressBar 进度条控件
        QProgressBar* progressBar = new QProgressBar(ui->tableWidget);
        progressBar->setRange(0, 100);
        int initPct = (p->fileSize > 0) ? (int)(sentBytes * 100 / p->fileSize) : 0;
        progressBar->setValue(initPct);
        progressBar->setAlignment(Qt::AlignCenter);
        progressBar->setFormat(QString("%1% (准备上传...)").arg(initPct));
        ui->tableWidget->setCellWidget(row, 3, progressBar);
        QCoreApplication::processEvents();


        // 3. 速度统计：1 秒滑动窗口 + 指数平滑，避免 TCP 突发导致数值剧烈跳动
        QElapsedTimer timer;
        timer.start();
        qint64 lastUpdateMs = 0;
        const qint64 kUpdateIntervalMs = 100;   // 界面刷新间隔：100ms
        const qint64 kWindowMs = 1000;          // 速度统计窗口：1 秒
        std::deque<std::pair<qint64, long long>> samples;   // (时刻, 累计已发送字节)
        samples.push_back(std::make_pair((qint64)0, sentBytes));
        double smoothSpeedKB = 0.0;             // 平滑后的显示速度（KB/s）
        bool speedInited = false;

        // 4. 循环读取文件并发送给服务器
        while(1)
        {
            int num = fread(sur.m_Stream, 1, ONEPAGE, pf);
            sur.m_StreamLen = num;
            if (num <= 0)
            {
                fclose(pf);
                break;
            }
            m_pKernel->sendData((char*)&sur, sizeof(sur));

            sentBytes += num;

            qint64 nowMs = timer.elapsed();
            if (nowMs - lastUpdateMs >= kUpdateIntervalMs || sentBytes >= p->fileSize)
            {
                lastUpdateMs = nowMs;
                samples.push_back(std::make_pair(nowMs, sentBytes));

                // 只保留最近 1 秒内的采样点（至少保留 2 个点才能算速度）
                while (samples.size() > 2 && nowMs - samples[1].first >= kWindowMs)
                    samples.pop_front();

                long long winBytes = samples.back().second - samples.front().second;
                double winSec = (samples.back().first - samples.front().first) / 1000.0;
                double instKB = (winSec > 0.001) ? (winBytes / 1024.0 / winSec) : 0.0;

                // 指数平滑：新值只占 25% 权重，旧值占 75%，曲线更平稳
                if (!speedInited)
                {
                    smoothSpeedKB = instKB;
                    speedInited = true;
                }
                else
                {
                    smoothSpeedKB = smoothSpeedKB * 0.75 + instKB * 0.25;
                }

                int pct = (p->fileSize > 0) ? (int)(sentBytes * 100 / p->fileSize) : 100;
                QString speedText = (smoothSpeedKB >= 1024.0)
                                        ? QString::asprintf("%.2f MB/s", smoothSpeedKB / 1024.0)
                                        : QString::asprintf("%.1f KB/s", smoothSpeedKB);

                progressBar->setValue(pct);
                progressBar->setFormat(QString("%1% (%2)").arg(pct).arg(speedText));
                QCoreApplication::processEvents(); // 刷新 UI
            }
        }

        // 5. 上传完毕，移除进度条并显示“上传成功”
        ui->tableWidget->removeCellWidget(row, 3);
        QTableWidgetItem* doneItem = new QTableWidgetItem("上传成功");
        doneItem->setFlags(doneItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, 3, doneItem);

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
        showStatusText(p->row, "下载失败");
        delete p;
        m_lstfileDown.erase(it);
        return;
    }

    // 把这一块数据写入本地文件
    p->pos += fwrite(psdr->fileStream, 1, psdr->fileStreamLen, p->pf);

    // 每 100ms（或全部收完时）刷新一次进度条和速度
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qint64 dt = nowMs - p->lastUpdateMs;
    if(dt >= 100 || p->pos >= p->fileSize)
    {
        int pct = (p->fileSize > 0) ? (int)(p->pos * 100 / p->fileSize) : 100;

        // 区间速度 → 指数平滑，避免数字剧烈跳动
        long long deltaBytes = p->pos - p->lastUpdatePos;
        double instKB = (dt > 0) ? (deltaBytes / 1024.0 / (dt / 1000.0)) : 0.0;
        if(p->speedKB <= 0.001) p->speedKB = instKB;
        else                    p->speedKB = p->speedKB * 0.75 + instKB * 0.25;

        QString speedText = (p->speedKB >= 1024.0)
                                ? QString::asprintf("%.2f MB/s", p->speedKB / 1024.0)
                                : QString::asprintf("%.1f KB/s", p->speedKB);

        // 不保存控件指针，每次从表格里取当下的进度条，控件被销毁也不会变成野指针
        QProgressBar* bar = nullptr;
        if(p->row >= 0 && p->row < ui->tableWidget->rowCount())
            bar = qobject_cast<QProgressBar*>(ui->tableWidget->cellWidget(p->row, 3));
        if(bar)
        {
            bar->setValue(pct);
            bar->setFormat(QString("下载中 %1% (%2)").arg(pct).arg(speedText));
        }

        p->lastUpdateMs  = nowMs;
        p->lastUpdatePos = p->pos;
        QCoreApplication::processEvents();   // 让界面实时刷新
    }

    // 全部收完
    if(p->pos >= p->fileSize)
    {
        fclose(p->pf);
        showStatusText(p->row, "下载成功");
        delete p;
        m_lstfileDown.erase(it);
        QMessageBox::information(this,"提示","下载成功");
    }

}

void Widget::showStatusText(int row, const QString& text)
{
    if(row < 0 || row >= ui->tableWidget->rowCount()) return;
    ui->tableWidget->removeCellWidget(row, 3);
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, 3, item);
}

long long Widget::fileIdAtRow(int row) const
{
    if(row < 0 || row >= ui->tableWidget->rowCount()) return -1;
    QTableWidgetItem* item = ui->tableWidget->item(row, 0);
    if(!item) return -1;
    QVariant v = item->data(Qt::UserRole);
    if(!v.isValid()) return -1;
    return v.toLongLong();
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
    // 从表格项里取文件 id（不再依赖外部缓存，永远不会不同步）
    long long fileId = fileIdAtRow(index);
    if(fileId < 0 || !ui->tableWidget->item(index, 0) || !ui->tableWidget->item(index, 1))
    {
        QMessageBox::critical(this,"错误","选中行数据不完整，请重新登录刷新文件列表");
        return;
    }
    STRU_DOWNLOADFILE_RQ sdr;
    sdr.userId = m_userId;
    sdr.fileId = fileId;
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
    if(!pf)
    {
        QMessageBox::critical(this,"错误","无法创建本地文件");
        return;
    }

    // 同一文件不允许重复下载：否则新进度条会销毁旧的，旧 f_down 里的 pBar 变成野指针
    for(auto d = m_lstfileDown.begin(); d != m_lstfileDown.end(); ++d)
    {
        if((*d)->fileId == sdr.fileId)
        {
            fclose(pf);
            QMessageBox::information(this,"提示","该文件正在下载中，请等待完成");
            return;
        }
    }

    f_down* p = new f_down;
    p->fileId = sdr.fileId;
    p->fileSize = atoll( ui->tableWidget->item(index,1)->text().toStdString().c_str());
    p->pf = pf;
    p->pos = 0;

    // 在该行“文件状态”列放一个进度条，让用户能看到下载过程
    p->row = index;
    QProgressBar* bar = new QProgressBar(ui->tableWidget);
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setAlignment(Qt::AlignCenter);
    bar->setFormat("准备下载...");
    ui->tableWidget->setCellWidget(index, 3, bar);

    p->lastUpdateMs  = QDateTime::currentMSecsSinceEpoch();
    p->lastUpdatePos = 0;
    p->speedKB       = 0.0;

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
    long long fileId = fileIdAtRow(index);
    if(fileId < 0) return;
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
    long long fileId = fileIdAtRow(index);
    if(fileId < 0) return;
    STRU_DELETEFILE_RQ ssr;
    ssr.fileId = fileId;
    ssr.userId = m_userId;
    ui->tableWidget->removeRow(index);
    m_pKernel->sendData((char*)&ssr,sizeof(ssr));
}

void Widget::initChatDialog()
{
    m_pChatDialog = new QDialog(this);
    m_pChatDialog->setWindowTitle("好友聊天");
    m_pChatDialog->resize(600, 420);

    QHBoxLayout* mainLayout = new QHBoxLayout(m_pChatDialog);

    // 左侧：好友列表
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(new QLabel("好友列表（点击选择聊天对象）："));
    m_pFriendListWidget = new QListWidget;
    leftLayout->addWidget(m_pFriendListWidget);

    // 右侧：消息记录 + 输入框 + 发送按钮
    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->addWidget(new QLabel("聊天记录："));
    m_pChatHistoryView = new QTextBrowser;
    rightLayout->addWidget(m_pChatHistoryView);

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    m_pChatInputEdit = new QLineEdit;
    m_pChatInputEdit->setPlaceholderText("输入消息按回车或点击发送...");
    QPushButton* btnSend = new QPushButton("发送");
    bottomLayout->addWidget(m_pChatInputEdit);
    bottomLayout->addWidget(btnSend);
    rightLayout->addLayout(bottomLayout);

    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 7);

    // 切换左侧选中的好友时，右侧自动显示与该好友的聊天记录
    connect(m_pFriendListWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current) return;
        long long fid = current->data(Qt::UserRole).toLongLong();
        m_pChatHistoryView->setHtml(m_mapChatHistory[fid]);
    });

    connect(btnSend, &QPushButton::clicked, this, &Widget::slot_sendChatMsg);
    connect(m_pChatInputEdit, &QLineEdit::returnPressed, this, &Widget::slot_sendChatMsg);
}

void Widget::on_pushButton_6_clicked()
{
    bool ok = false;
    QString friendName = QInputDialog::getText(this, "添加好友", "请输入要添加的好友用户名：", QLineEdit::Normal, "", &ok);
    if (!ok || friendName.trimmed().isEmpty()) return;

    STRU_ADDFRIEND_RQ sar;
    sar.m_userId = m_userId;
    strncpy(sar.m_friendName, friendName.trimmed().toUtf8().constData(), MAXSIZE - 1);
    m_pKernel->sendData((char*)&sar, sizeof(sar));
}

void Widget::on_pushButton_7_clicked()
{
    m_pChatDialog->show();
    m_pChatDialog->raise();
    m_pChatDialog->activateWindow();
}

void Widget::slots_addfriendrs(STRU_ADDFRIEND_RS *psar)
{
    if (psar->m_ret == ADDFRIEND_NO_USER) {
        QMessageBox::warning(this, "提示", "该用户名不存在！");
    } else if (psar->m_ret == ADDFRIEND_EXIST) {
        QMessageBox::information(this, "提示", "对方已经是你的好友（或不能添加自己）！");
    } else if (psar->m_ret == ADDFRIEND_SUC) {
        QString fname = QString::fromUtf8(psar->m_friendName);
        QMessageBox::information(this, "提示", QString("添加好友 [%1] 成功！").arg(fname));
        if (m_mapFriends.find(psar->m_friendId) == m_mapFriends.end()) {
            m_mapFriends[psar->m_friendId] = fname;
            QListWidgetItem* item = new QListWidgetItem(fname);
            item->setData(Qt::UserRole, QVariant::fromValue(psar->m_friendId));
            m_pFriendListWidget->addItem(item);
        }
    }
}

void Widget::slots_friendlistrs(STRU_GETFRIENDLIST_RS *psgr)
{
    for (int i = 0; i < psgr->m_friendNum; i++) {
        long long fid = psgr->m_friendInfo[i].m_friendId;
        QString fname = QString::fromUtf8(psgr->m_friendInfo[i].m_friendName);
        if (m_mapFriends.find(fid) == m_mapFriends.end()) {
            m_mapFriends[fid] = fname;
            QListWidgetItem* item = new QListWidgetItem(fname);
            item->setData(Qt::UserRole, QVariant::fromValue(fid));
            m_pFriendListWidget->addItem(item);
        }
    }
}

void Widget::slot_sendChatMsg()
{
    QListWidgetItem* item = m_pFriendListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(m_pChatDialog, "提示", "请先在左侧选择一个好友！");
        return;
    }
    QString text = m_pChatInputEdit->text().trimmed();
    if (text.isEmpty()) return;

    long long friendId = item->data(Qt::UserRole).toLongLong();
    STRU_CHAT_RQ scr{};
    memset(scr.m_sendUserName, 0, sizeof(scr.m_sendUserName));
    memset(scr.m_content, 0, sizeof(scr.m_content));
    scr.m_sendUserId = m_userId;
    scr.m_recvUserId = friendId;
    strncpy(scr.m_content, text.toUtf8().constData(), CHATLEN - 1);
    m_pKernel->sendData((char*)&scr, sizeof(scr));

    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_mapChatHistory[friendId] += QString("<p><font color='blue'><b>我 [%1]:</b></font><br>%2</p>")
                                      .arg(timeStr, text.toHtmlEscaped());
    m_pChatHistoryView->setHtml(m_mapChatHistory[friendId]);
    m_pChatInputEdit->clear();
}

void Widget::slots_chatrq(STRU_CHAT_RQ *pscr)
{
    long long senderId = pscr->m_sendUserId;
    QString senderName = QString::fromUtf8(pscr->m_sendUserName);
    QString content = QString::fromUtf8(pscr->m_content);

    // 如果发送者不在本地好友列表中，自动加入列表
    if (m_mapFriends.find(senderId) == m_mapFriends.end()) {
        m_mapFriends[senderId] = senderName;
        QListWidgetItem* item = new QListWidgetItem(senderName);
        item->setData(Qt::UserRole, QVariant::fromValue(senderId));
        m_pFriendListWidget->addItem(item);
    }

    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_mapChatHistory[senderId] += QString("<p><font color='green'><b>%1 [%2]:</b></font><br>%3</p>")
                                      .arg(senderName, timeStr, content.toHtmlEscaped());

    // 如果当前正选中该好友，立即刷新右侧聊天框
    QListWidgetItem* cur = m_pFriendListWidget->currentItem();
    if (cur && cur->data(Qt::UserRole).toLongLong() == senderId) {
        m_pChatHistoryView->setHtml(m_mapChatHistory[senderId]);
    }
    // 自动弹出聊天窗提醒
    m_pChatDialog->show();
}

void Widget::slots_chatrs(STRU_CHAT_RS *pscr)
{
    if (pscr->m_ret == CHAT_OFFLINE) {
        long long fid = pscr->m_recvUserId;
        m_mapChatHistory[fid] += "<p><font color='gray'><i>[系统提示：对方当前不在线]</i></font></p>";
        QListWidgetItem* cur = m_pFriendListWidget->currentItem();
        if (cur && cur->data(Qt::UserRole).toLongLong() == fid) {
            m_pChatHistoryView->setHtml(m_mapChatHistory[fid]);
        }
    }
}

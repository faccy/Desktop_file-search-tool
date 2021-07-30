#include "MainWindow.h"
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QFileIconProvider>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);
    QFile file(":/beautify.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());
    qApp->setStyleSheet(styleSheet);

    this->setWindowTitle("桌面文件查找");
    ui->label->setText("文件查找");
    ui->searchEdit->setFixedHeight(25);
    ui->fileTableW->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->fileTableW->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTableW->verticalHeader()->setVisible(false);
    ui->fileTableW->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->fileTableW->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->fileTableW->setColumnCount(4);
    ui->fileTableW->setShowGrid(false);
    ui->fileTableW->setFocusPolicy(Qt::NoFocus);//去除选中虚线框

    QStringList headerText;
    headerText << "名称" << "路径" << "大小" << "修改时间";
    ui->fileTableW->setHorizontalHeaderLabels(headerText);
    ui->fileTableW->viewport()->installEventFilter(this);
    desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::realTimeSearch);
    fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath(desktopPath);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::realTimeSearch);
}

//汉字转拼音
void ChineseConvertPy(QString& chinese, QString& letter) {
    const int spell_value[23] = { 0xb0a1, 0xb0c5, 0xb2c1, 0xb4ee, 0xb6ea, 0xb7a2, 0xb8c1,
    0xb9fe, 0xbbf7, 0xbfa6, 0xc0ac, 0xc2e8, 0xc4c3, 0xc5b6, 0xc5be, 0xc6da, 0xc8bb,
    0xc8f6, 0xcbfa, 0xcdda, 0xcef4, 0xd1b9, 0xd4d1 };

    const char spell_dict[23] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'w', 'x', 'y', 'z' };

    QByteArray ba = chinese.toLocal8Bit();
    std::string dest_chinese = ba.constData();
    const int length = dest_chinese.length();
    for (int j = 0, chrasc = 0; j < length;) {
        if (dest_chinese[j] >= 0 && dest_chinese[j] < 128) {
            letter += dest_chinese[j];
            j++;
            continue;
        }
        chrasc = (dest_chinese[j] + 256) * 256 + dest_chinese[j + 1] + 256;
        for (int i = 22; i >= 0; --i) {
            if (chrasc >= spell_value[i]) {
                letter += spell_dict[i];
                break;
            }
        }
        j += 2;
    }
}


//判断是否与要查找的文件相似
int isSimilar(QString filename, QString inputName) {
    int i = 0, j = 0;
    int m = inputName.size(), n = filename.size();
    bool isPreSame = false; //前一个字符是否匹配
    int res1 = 0;
    if (inputName == filename) {
        return 2 * m + 1;
    }
    filename = filename.toLower();
    inputName = inputName.toLower();
    if (inputName == filename) {
        return 2 * m;
    }
    while (i < m && j < n) {
        if (inputName[i] == filename[j]) {
            res1++;
            if (isPreSame) res1++;
            isPreSame = true;
            i++;
            j++;
        }
        else {
            isPreSame = false;
            i++;
        }
    }
    //模糊匹配
    int res2 = 0;
    i = j = 0;
    while (i < m && j < n) {
        if (inputName[i] == filename[j]) {
            res2++;
            i++;
        }
        j++;
    }
    int res = res1 > res2 ? res1 : res2;
    return res;
}

//把文件大小从字节进行转换
QString transSize(qint64 size) {
    qint64 perK = 1024;
    qint64 perM = perK * perK;
    qint64 perG = perM * perK;
    qreal K = 0, M = 0, G = 0;
    QString formatSize = "";
    G = size * 1.0 / perG;
    if (G > 1) {
        formatSize = QString::number(G, 'f', 2);
        formatSize += "G";
        return formatSize;
    }
    else {
        M = size * 1.0 / perM;
        if (M > 1) {
            formatSize = QString::number(M, 'f', 2);
            formatSize += "M";
            return formatSize;
        }
        else {
            K = size * 1.0 / perK;
            if (K > 1) {
                formatSize = QString::number(K, 'f', 2);
                formatSize += "K";
                return formatSize;
            }
            else {
                formatSize = QString::number(size);
                formatSize += "B";
                return formatSize;
            }
        }
    }
}


bool Cmp(const QPair<QString, int>& a, const QPair<QString, int>& b) {
    return a.second > b.second;
}

void MainWindow::realTimeSearch() {
    ui->fileTableW->clearContents();
    for (int i = 0; i < preVecSize; ++i) {
        ui->fileTableW->removeRow(0);
    }
    row = 0;
    QString inputName = ui->searchEdit->text();
    QDir* desktopDir = new QDir(desktopPath);
    QStringList fileNames = desktopDir->entryList();
    QVector<QPair<QString, int>> resFiles;
    for (auto filename : fileNames) {
        int value1 = isSimilar(filename, inputName);
        QString chineseQStr;
        ChineseConvertPy(filename, chineseQStr);
        int value2 = isSimilar(chineseQStr, inputName);
        int value = value1 > value2 ? value1 : value2;
        if (value > 1) {
            resFiles.push_back(QPair<QString, int>(filename, value));
        }
    }
    std::sort(resFiles.begin(), resFiles.end(), Cmp);
    preVecSize = resFiles.size();
    for (auto ele : resFiles) {
        QString filename = ele.first;
        QString wholePath = desktopPath + "/" + filename;
        QFileInfo fileInfo(wholePath);
        QFileIconProvider fileIcon;
        QIcon insertIcon = fileIcon.icon(fileInfo);
        ui->fileTableW->insertRow(row);
        ui->fileTableW->setItem(row, 0, new QTableWidgetItem(insertIcon, filename));
        ui->fileTableW->setItem(row, 1, new QTableWidgetItem(fileInfo.absoluteFilePath()));
        QString fileSize = transSize(fileInfo.size());
        if (fileInfo.isFile()) {
            ui->fileTableW->setItem(row, 2, new QTableWidgetItem(fileSize));
        }
        ui->fileTableW->setItem(row++, 3, new QTableWidgetItem(fileInfo.lastModified()
            .toString("yyyy/MM/dd hh:mm:ss")));
    }
    ui->fileTableW->selectRow(0); //默认选中第一行，使最匹配的项高亮显示
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->fileTableW->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton) {
                QTableWidgetItem* itemNow = ui->fileTableW->currentItem();
                QMenu* rightMenu = new QMenu;
                rightMenu->addAction("打开", this, [=]() {
                    QString wholePath = desktopPath + "/" + itemNow->text();
                    QDesktopServices::openUrl(QUrl(wholePath));
                    });
                rightMenu->addAction("删除", this, [=]() {
                    QMessageBox::StandardButton result;
                    result = QMessageBox::question(this, "提示", "确认要删除吗?",
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    if (result == QMessageBox::Yes) {
                        QString wholePath = desktopPath + "/" + itemNow->text();
                        deleteFileOrFolder(wholePath);
                    }
                    });
                rightMenu->exec(QCursor::pos());
                return false;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

bool MainWindow::deleteFileOrFolder(const QString& strPath) {
    if (strPath.isEmpty() || !QDir().exists(strPath)) {
        return false;
    }
    QFileInfo FileInfo(strPath);
    if (FileInfo.isFile()) {
        QFile::remove(strPath);
    }
    else if (FileInfo.isDir()) {
        QDir qDir(strPath);
        qDir.removeRecursively();
    }
    return true;
}

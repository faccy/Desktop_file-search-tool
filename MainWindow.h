#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include <QFileSystemWatcher>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = Q_NULLPTR);
    bool eventFilter(QObject*, QEvent*);
    bool deleteFileOrFolder(const QString&);

private slots:
    void realTimeSearch(); //实时查找

private:
    Ui::MainWindowClass* ui;
    QString desktopPath;
    int row = 0;
    int preVecSize = 0;
    QFileSystemWatcher* fileWatcher;
};

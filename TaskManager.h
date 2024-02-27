#include <QtWidgets/QMainWindow>
#include "ui_TaskManager.h"
#include <QtWidgets>
#include <windows.h>
#include <tlhelp32.h>
#include <QMutex>
#include <QThread>
#include <QCoreApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QMutexLocker>
#include <QtCharts>
#include"SystemMonitorWidget.h"
#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QtCharts>
#include"ProcessInfoThread.h"
#include"mainwindow.h"
class TaskManager : public QMainWindow
{
    Q_OBJECT

public:
    TaskManager(QWidget* parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("TaskManager Team leader:Gu Congzhi Sno:2110410115");
        setupUi();
        // 设置窗口初始大小为800x600
        resize(800, 600);
        connect(aboutAction, &QAction::triggered, this, &TaskManager::onaboutActionClicked);
        connect(fileAction, &QAction::triggered, this, &TaskManager::onFileActionClicked);
        connect(refreshAction, &QAction::triggered, this, &TaskManager::onrefreshActionClicked);
        connect(performanceAction, &QAction::triggered, this, &TaskManager::onsettingActionClicked);
        processList->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(processList, &QTreeWidget::customContextMenuRequested, this, &TaskManager::showContextMenu);

        processMonitorThread = new ProcessMonitorThread(this);
        connect(processMonitorThread, &ProcessMonitorThread::processListChanged, this, &TaskManager::onProcessListChanged);
        processMonitorThread->start();
      //  connect(processList, &QTreeWidget::itemDoubleClicked, this, &TaskManager::showRealTimeInfoWindow);
        connect(processList, &QTreeWidget::itemClicked, this, &TaskManager::handleProcessListClicked);
    }
    ~TaskManager() {
        processMonitorThread->requestInterruption();
        processMonitorThread->wait();
        delete processMonitorThread;
    }
    void clearProcessList()
    {
        processList->clear();
        processList->setHeaderLabels(QStringList() << "Name" << "PID" << "Threads" << "Parent PID"<<"Priority"<<"cntThreads");
    }
    void addProcessToList(const PROCESSENTRY32& processEntry)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();

        // 在树项中设置进程信息
        item->setText(0, QString::fromWCharArray(processEntry.szExeFile));
        item->setText(1, QString::number(processEntry.th32ProcessID));
        item->setText(2, QString::number(processEntry.cntThreads));
        item->setText(3, QString::number(processEntry.th32ParentProcessID));
        item->setText(4, QString::number(processEntry.pcPriClassBase));
        item->setText(5, QString::number(processEntry.cntThreads));
        // 根据父进程ID查找父项
        QList<QTreeWidgetItem*> parentItems = processList->findItems(QString::number(processEntry.th32ParentProcessID),
            Qt::MatchExactly, 1);

        if (!parentItems.isEmpty())
        {
            // 获取父项
            QTreeWidgetItem* parentItem = parentItems.first();

            // 将子项添加到对应的父项下
            parentItem->addChild(item);
        }
        else
        {
            // 如果找不到父项，则将其作为顶级项添加
            processList->addTopLevelItem(item);
        }
       
    }
    QString getExecutablePathByPid(DWORD pid)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess != NULL)
        {
            WCHAR buffer[MAX_PATH];
            DWORD size = sizeof(buffer) / sizeof(buffer[0]);

            if (QueryFullProcessImageName(hProcess, 0, buffer, &size))
            {
                CloseHandle(hProcess);
                return QString::fromWCharArray(buffer);
            }

            CloseHandle(hProcess);
        }

        return QString();
    }
    void openFileManagerByPid(DWORD pid)
    {
        QString filePath = getExecutablePathByPid(pid);

        if (!filePath.isEmpty())
        {
            QFileInfo fileInfo(filePath);
            QString directoryPath = fileInfo.path();

            // 打开文件管理器并选择文件
            QDesktopServices::openUrl(QUrl::fromLocalFile(directoryPath));
        }
        else
        {
            qDebug() << "Unable to retrieve executable path for PID:" << pid;
        }
    }
private slots:
    void TaskManager::openFileLocation()
    {
        QTreeWidgetItem* selectedItem = processList->currentItem();
        if (selectedItem)
        {
            // 获取选中进程的PID
            QString processIDString = selectedItem->text(1);
            bool ok;
            DWORD processID = processIDString.toUInt(&ok);

            if (ok)
            {
                // 获取进程的可执行文件路径
                QString filePath = getExecutablePathByPid(processID);
                qDebug() << filePath;
                if (!filePath.isEmpty())
                {
                    QFileInfo fileInfo(filePath);
                    QString directoryPath = fileInfo.path();

                    // 打开文件管理器并选择文件
                    QDesktopServices::openUrl(QUrl::fromLocalFile(directoryPath));
                }
                else
                {
                    qDebug() << "Unable to retrieve executable path for PID:" << processID;
                }
            }
        }
    }
    void handleProcessListClicked(QTreeWidgetItem* item)
    {
        // 在这里获取点击的进程信息，然后调用 additionalInfoWidget 的 updateInfo 函数
        QString processIDString = item->text(1);
        QString prccessname = item->text(0);
        bool ok;
        DWORD processID = processIDString.toUInt(&ok);

        if (ok)
        {
            // 获取并显示进程的 CPU 和内存信息（这里是示例，具体的获取信息的方法需要根据实际情况修改）
            QString prccessname = item->text(0);
            additionalInfoWidget->changupdateInfo(processID, prccessname);
        }
    }
    void refreshProcessList()
    {
        // QMutexLocker locker(&mutex);

         // 这里是更新进程列表的逻辑，简化为清空列表
       
        clearProcessList();

        // 获取进程快照
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            qDebug() << "Failed to create snapshot.";
            return;
        }

        // 遍历进程
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                addProcessToList(processEntry);
            } while (Process32Next(hSnapshot, &processEntry));
        }

        CloseHandle(hSnapshot);
    }
    void showContextMenu(const QPoint& pos)
    {
        //  QMutexLocker locker(&mutex);
        QTreeWidgetItem* selectedItem = processList->itemAt(pos);
        if (selectedItem)
        {
            // 获取选中进程的PID
            QString processIDString = selectedItem->text(1);
            bool ok;
            DWORD processID = processIDString.toUInt(&ok);
            if (ok)
            {
                QMenu contextMenu(this);
                QAction* terminateAction = new QAction("Terminate Process", this);
                // 连接结束进程操作的槽函数
                connect(terminateAction, &QAction::triggered, this, [=]() {
                    WorkerThread* workerThread = new WorkerThread(processID, this);

                    //// 连接结束进程操作完成的信号
                    connect(workerThread, &WorkerThread::operationCompleted, this, [=]() {
                        //    // 在这里更新UI或执行其他与UI相关的操作
                        refreshProcessList();

                        //    // 释放线程资源
                        workerThread->deleteLater();
                        });

                    //// 启动线程执行结束进程的操作
                    workerThread->start();
                    });
                
                // ... 其他代码

                QAction* openFileLocationAction = new QAction("Open File Location", this);
                connect(openFileLocationAction, &QAction::triggered, this, &TaskManager::openFileLocation);
                contextMenu.addAction(openFileLocationAction);
                contextMenu.addAction(terminateAction);
                contextMenu.exec(processList->mapToGlobal(pos));
            }
        }
    }
    void onProcessListChanged(const QList<PROCESSENTRY32>& newProcessList)
    {
        // 使用新的进程列表更新UI
        qDebug() << "Process list changed. New size:" << newProcessList.size();

        // 使用新的进程列表更新UI
        clearProcessList();
        for (const PROCESSENTRY32& processEntry : newProcessList)
        {
            qDebug() << "Adding process:" << processEntry.szExeFile << processEntry.th32ProcessID;
            addProcessToList(processEntry);
        }
    }
    void onFileActionClicked()
    {
    
        QAction* runNewTaskAction = new QAction("Run New Task", this);
        QAction* exitAction = new QAction("Exit", this);
        // 创建一个菜单，添加子选项
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // 添加分隔线
        fileMenu->addAction(exitAction);

        // 显示菜单
        fileMenu->exec(QCursor::pos());
    }
    void onrefreshActionClicked()
    {

        QAction* runNewTaskAction = new QAction("Refresh Now", this);
        QAction* exitAction = new QAction("Refresh Speed", this);
        // 创建一个菜单，添加子选项
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // 添加分隔线
        fileMenu->addAction(exitAction);

        // 显示菜单
        fileMenu->exec(QCursor::pos());
    }
    void onsettingActionClicked()
    {

        QAction* runNewTaskAction = new QAction("Max window", this);
        QAction* exitAction = new QAction("Fixed window", this);
        // 创建一个菜单，添加子选项
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // 添加分隔线
        fileMenu->addAction(exitAction);

        // 显示菜单
        fileMenu->exec(QCursor::pos());
    }
    void onaboutActionClicked()
    {

        QMessageBox msgBox;
        msgBox.setWindowTitle("About");
        msgBox.setText("The program is implemented by qt and C++ and written by Gu Congzhi.");

        // 添加一个带有标签的布局
       // Q//VBoxLayout* layout = new QVBoxLayout;

       // QLabel* label = new QLabel("The program is implemented by qt and C++ and written by Gu Congzhi.");
       // layout->addWidget(label);

       // msgBox.setLayout(layout);

        // 显示弹窗
        msgBox.exec();
    }
private:
    void setupUi()
    {
        // 创建主窗口部件
        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建布局管理器
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        QToolBar* toolbar = addToolBar("Toolbar");

        // 添加按钮
        fileAction = toolbar->addAction("File");
        performanceAction = toolbar->addAction("Setting");
        refreshAction = toolbar->addAction("Refresh");
        aboutAction = toolbar->addAction("About");
        QAction* ShutdownAction = toolbar->addAction("Shutdown");
        // 将工具栏添加到布局
        layout->addWidget(toolbar);

        // 创建 Tab Widget
        tabWidget = new QTabWidget(this);

        // 创建进程列表页面
        QWidget* processPage = new QWidget(this);

        // 创建水平分隔控件
        QSplitter* splitter = new QSplitter(processPage);

        // 将进程列表添加到分隔控件中
        processList = new QTreeWidget(this);
        processList->setColumnCount(4);
        processList->setHeaderLabels(QStringList() << "Name" << "PID" << "Threads" << "Parent PID" << "Priority" << "cntThreads");
        splitter->addWidget(processList);

        // 创建包含两个 QLabel 的 QWidget
        
          // 创建包含两个 QLabel 的 QWidget
        additionalInfoWidget = new AdditionalInfoWidget(this);

        // 将包含两个 Label 的 QWidget 添加到分隔控件中
        splitter->addWidget(additionalInfoWidget);
        // 设置分隔控件的方向为水平
        splitter->setOrientation(Qt::Horizontal);

        // 将分隔控件添加到进程列表页面
        QVBoxLayout* processLayout = new QVBoxLayout(processPage);
        processLayout->addWidget(splitter);

        // 将进程列表页面添加到 Tab Widget 中
        tabWidget->addTab(processPage, "Processes");

        // 创建性能信息页面
        monitor = new SystemMonitorWidget(this);
        tabWidget->addTab(monitor, "Performance");
       
        user = new MainWindow(this);
        tabWidget->addTab(user, "User");
        tabWidget->setStyleSheet("QTabBar::tab { width: 150px; height: 40px; }");
        // 将 Tab Widget 添加到布局
        layout->addWidget(tabWidget);

        // 设置布局管理器
        centralWidget->setLayout(layout);
    }
    QTreeWidget* processList;
    QTabWidget* tabWidget;
    SystemMonitorWidget *monitor;
    ProcessMonitorThread* processMonitorThread;
    AdditionalInfoWidget* additionalInfoWidget;
    QAction* fileAction;
    MainWindow* user;
    QAction* performanceAction;
    QAction* refreshAction;
    QAction* aboutAction;
};

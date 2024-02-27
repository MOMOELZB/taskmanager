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
        // ���ô��ڳ�ʼ��СΪ800x600
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

        // �����������ý�����Ϣ
        item->setText(0, QString::fromWCharArray(processEntry.szExeFile));
        item->setText(1, QString::number(processEntry.th32ProcessID));
        item->setText(2, QString::number(processEntry.cntThreads));
        item->setText(3, QString::number(processEntry.th32ParentProcessID));
        item->setText(4, QString::number(processEntry.pcPriClassBase));
        item->setText(5, QString::number(processEntry.cntThreads));
        // ���ݸ�����ID���Ҹ���
        QList<QTreeWidgetItem*> parentItems = processList->findItems(QString::number(processEntry.th32ParentProcessID),
            Qt::MatchExactly, 1);

        if (!parentItems.isEmpty())
        {
            // ��ȡ����
            QTreeWidgetItem* parentItem = parentItems.first();

            // ��������ӵ���Ӧ�ĸ�����
            parentItem->addChild(item);
        }
        else
        {
            // ����Ҳ������������Ϊ���������
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

            // ���ļ���������ѡ���ļ�
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
            // ��ȡѡ�н��̵�PID
            QString processIDString = selectedItem->text(1);
            bool ok;
            DWORD processID = processIDString.toUInt(&ok);

            if (ok)
            {
                // ��ȡ���̵Ŀ�ִ���ļ�·��
                QString filePath = getExecutablePathByPid(processID);
                qDebug() << filePath;
                if (!filePath.isEmpty())
                {
                    QFileInfo fileInfo(filePath);
                    QString directoryPath = fileInfo.path();

                    // ���ļ���������ѡ���ļ�
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
        // �������ȡ����Ľ�����Ϣ��Ȼ����� additionalInfoWidget �� updateInfo ����
        QString processIDString = item->text(1);
        QString prccessname = item->text(0);
        bool ok;
        DWORD processID = processIDString.toUInt(&ok);

        if (ok)
        {
            // ��ȡ����ʾ���̵� CPU ���ڴ���Ϣ��������ʾ��������Ļ�ȡ��Ϣ�ķ�����Ҫ����ʵ������޸ģ�
            QString prccessname = item->text(0);
            additionalInfoWidget->changupdateInfo(processID, prccessname);
        }
    }
    void refreshProcessList()
    {
        // QMutexLocker locker(&mutex);

         // �����Ǹ��½����б���߼�����Ϊ����б�
       
        clearProcessList();

        // ��ȡ���̿���
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            qDebug() << "Failed to create snapshot.";
            return;
        }

        // ��������
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
            // ��ȡѡ�н��̵�PID
            QString processIDString = selectedItem->text(1);
            bool ok;
            DWORD processID = processIDString.toUInt(&ok);
            if (ok)
            {
                QMenu contextMenu(this);
                QAction* terminateAction = new QAction("Terminate Process", this);
                // ���ӽ������̲����Ĳۺ���
                connect(terminateAction, &QAction::triggered, this, [=]() {
                    WorkerThread* workerThread = new WorkerThread(processID, this);

                    //// ���ӽ������̲�����ɵ��ź�
                    connect(workerThread, &WorkerThread::operationCompleted, this, [=]() {
                        //    // ���������UI��ִ��������UI��صĲ���
                        refreshProcessList();

                        //    // �ͷ��߳���Դ
                        workerThread->deleteLater();
                        });

                    //// �����߳�ִ�н������̵Ĳ���
                    workerThread->start();
                    });
                
                // ... ��������

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
        // ʹ���µĽ����б����UI
        qDebug() << "Process list changed. New size:" << newProcessList.size();

        // ʹ���µĽ����б����UI
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
        // ����һ���˵��������ѡ��
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // ��ӷָ���
        fileMenu->addAction(exitAction);

        // ��ʾ�˵�
        fileMenu->exec(QCursor::pos());
    }
    void onrefreshActionClicked()
    {

        QAction* runNewTaskAction = new QAction("Refresh Now", this);
        QAction* exitAction = new QAction("Refresh Speed", this);
        // ����һ���˵��������ѡ��
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // ��ӷָ���
        fileMenu->addAction(exitAction);

        // ��ʾ�˵�
        fileMenu->exec(QCursor::pos());
    }
    void onsettingActionClicked()
    {

        QAction* runNewTaskAction = new QAction("Max window", this);
        QAction* exitAction = new QAction("Fixed window", this);
        // ����һ���˵��������ѡ��
        QMenu* fileMenu = new QMenu(this);
        fileMenu->addAction(runNewTaskAction);
        fileMenu->addSeparator(); // ��ӷָ���
        fileMenu->addAction(exitAction);

        // ��ʾ�˵�
        fileMenu->exec(QCursor::pos());
    }
    void onaboutActionClicked()
    {

        QMessageBox msgBox;
        msgBox.setWindowTitle("About");
        msgBox.setText("The program is implemented by qt and C++ and written by Gu Congzhi.");

        // ���һ�����б�ǩ�Ĳ���
       // Q//VBoxLayout* layout = new QVBoxLayout;

       // QLabel* label = new QLabel("The program is implemented by qt and C++ and written by Gu Congzhi.");
       // layout->addWidget(label);

       // msgBox.setLayout(layout);

        // ��ʾ����
        msgBox.exec();
    }
private:
    void setupUi()
    {
        // ���������ڲ���
        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // �������ֹ�����
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        QToolBar* toolbar = addToolBar("Toolbar");

        // ��Ӱ�ť
        fileAction = toolbar->addAction("File");
        performanceAction = toolbar->addAction("Setting");
        refreshAction = toolbar->addAction("Refresh");
        aboutAction = toolbar->addAction("About");
        QAction* ShutdownAction = toolbar->addAction("Shutdown");
        // ����������ӵ�����
        layout->addWidget(toolbar);

        // ���� Tab Widget
        tabWidget = new QTabWidget(this);

        // ���������б�ҳ��
        QWidget* processPage = new QWidget(this);

        // ����ˮƽ�ָ��ؼ�
        QSplitter* splitter = new QSplitter(processPage);

        // �������б���ӵ��ָ��ؼ���
        processList = new QTreeWidget(this);
        processList->setColumnCount(4);
        processList->setHeaderLabels(QStringList() << "Name" << "PID" << "Threads" << "Parent PID" << "Priority" << "cntThreads");
        splitter->addWidget(processList);

        // ������������ QLabel �� QWidget
        
          // ������������ QLabel �� QWidget
        additionalInfoWidget = new AdditionalInfoWidget(this);

        // ���������� Label �� QWidget ��ӵ��ָ��ؼ���
        splitter->addWidget(additionalInfoWidget);
        // ���÷ָ��ؼ��ķ���Ϊˮƽ
        splitter->setOrientation(Qt::Horizontal);

        // ���ָ��ؼ���ӵ������б�ҳ��
        QVBoxLayout* processLayout = new QVBoxLayout(processPage);
        processLayout->addWidget(splitter);

        // �������б�ҳ����ӵ� Tab Widget ��
        tabWidget->addTab(processPage, "Processes");

        // ����������Ϣҳ��
        monitor = new SystemMonitorWidget(this);
        tabWidget->addTab(monitor, "Performance");
       
        user = new MainWindow(this);
        tabWidget->addTab(user, "User");
        tabWidget->setStyleSheet("QTabBar::tab { width: 150px; height: 40px; }");
        // �� Tab Widget ��ӵ�����
        layout->addWidget(tabWidget);

        // ���ò��ֹ�����
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

// mainwindow.cpp

#include "mainwindow.h"
#include <QStandardPaths>
#include <QDebug>
#include <QProcess>
#include <windows.h>
#include <winbase.h>
#include <TlHelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // 添加定时器，每秒更新一次系统信息
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSystemInfo);
    timer->start(10000);  // 每秒触发一次
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建一个垂直布局容器
    centralLayout = new QVBoxLayout;

    // 创建一个树形控件，用于显示进程树
    treeWidget = new QTreeWidget(this);
    treeWidget->setColumnCount(6);
    treeWidget->setHeaderLabels(QStringList() << "User" << "CPU"<<"Memory"<<"Disk"<<"Net"<<"GPU");
    processItem = createProcessTreeWidgetItem(QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section("/", -1), "");
    treeWidget->addTopLevelItem(processItem);
    treeWidget->collapseItem(processItem);

    // 添加树形控件到布局中
    centralLayout->addWidget(treeWidget);

    // 创建一个中心窗口，并将布局设置给这个中心窗口
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(centralLayout);

    // 将这个中心窗口设置为主窗口的中央部分
    setCentralWidget(centralWidget);

    // 设置初始窗口大小
    resize(800, 600);  // 以适当的宽度和高度替换这些值

    // 初始化底部信息
    updateSystemInfo();
}

void MainWindow::updateSystemInfo()
{
    // 获取真实系统信息
    QString cpuUsage = getCPUUsage();
    QString memoryUsage = getMemoryUsage();
    QString diskUsage = getDiskUsage();
    QString networkUsage = getNetworkUsage();
    QString gpuUsage = getGPUUsage();

    // 更新真实系统信息
    processItem->setText(0, QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section("/", -1));
    processItem->setText(1, cpuUsage);
    processItem->setText(2, memoryUsage);
    processItem->setText(3, diskUsage);
    processItem->setText(4, networkUsage);
    processItem->setText(5, gpuUsage);
    // 更新进程信息
    updateProcessInfo();
}

void MainWindow::updateProcessInfo()
{
    // 清空进程树
    processItem->takeChildren();

    // 使用CreateToolhelp32Snapshot获取系统进程信息
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &processEntry))
        {
            do
            {
                QString processName = QString::fromWCharArray(processEntry.szExeFile);
                QString processID = QString::number(processEntry.th32ProcessID);

                QTreeWidgetItem *processSubItem = createProcessTreeWidgetItem(processName, processID);
                processItem->addChild(processSubItem);
            } while (Process32Next(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
    }
    else
    {
        qDebug() << "Failed to create process snapshot";
    }
}

QString MainWindow::getCPUUsage()
{
    // 使用 Windows API 获取 CPU 使用率
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_FMT_COUNTERVALUE value;

    if (PdhOpenQuery(nullptr, 0, &query) == ERROR_SUCCESS)
    {
        if (PdhAddCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter) == ERROR_SUCCESS)
        {
            if (PdhCollectQueryData(query) == ERROR_SUCCESS)
            {
                Sleep(1000);  // 延迟一秒再采集数据
                if (PdhCollectQueryData(query) == ERROR_SUCCESS)
                {
                    if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS)
                    {
                        return QString::number(value.doubleValue+5, 'f', 2) + "%";
                    }
                }
            }
            PdhRemoveCounter(counter);
        }
        PdhCloseQuery(query);
    }

    return "N/A";
}

QString MainWindow::getMemoryUsage()
{
    // 使用 Windows API 获取内存使用情况
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus))
    {
        return QString::number(memoryStatus.dwMemoryLoad) + "%";
    }

    return "N/A";
}

QString MainWindow::getDiskUsage()
{
    // 使用 Windows API 获取磁盘调用速度
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(nullptr, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
    {
        // 模拟磁盘调用速度，实际应该使用真实的磁盘读写速度获取方法
        qint64 diskCallsPerSecond = rand() % 100;

        // 转换为 MB/s
        double diskUsage = (diskCallsPerSecond * 1.0) / (1024 * 1024);

        return QString::number(diskUsage, 'f', 2) + " MB/s";
    }

    return "N/A";
}

QString MainWindow::getNetworkUsage()
{
    // 使用 Windows API 获取网络使用情况
    MIB_IFROW ifRow;
    ifRow.dwIndex = 1; // 网络接口的索引，此处使用默认的以太网接口
    if (GetIfEntry(&ifRow) == NO_ERROR)
    {
        return QString::number(ifRow.dwInOctets / 1024) + " KB/s";
    }

    return "N/A";
}

QString MainWindow::getGPUUsage()
{
    // 使用 NVIDIA System Management Interface (nvidia-smi) 获取 GPU 使用率
    QProcess process;
    process.start("nvidia-smi", QStringList() << "--query-gpu=utilization.gpu --format=csv,noheader,nounits");

    if (process.waitForFinished())
    {
        QByteArray result = process.readAll();
        result = result.trimmed();

        bool ok;
        int gpuUsage = result.toInt(&ok);

        if (ok)
        {
            return QString::number(gpuUsage) + "%";
        }
    }

    return "N/A";
}

QTreeWidgetItem *MainWindow::createProcessTreeWidgetItem(const QString &label, const QString &value)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, label);
    item->setText(1, value);
    return item;
}

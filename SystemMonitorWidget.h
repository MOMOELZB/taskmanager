#pragma once
#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QtCharts>
#include "SystemMonitor.h"

class SystemMonitorWidget : public QWidget
{
    Q_OBJECT
public:
    SystemMonitorWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        // 创建主布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        QLabel* cpuinfo = new QLabel();
        QLabel* memoryinfo = new QLabel();
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        updateCpuPerformanceLabel(cpuinfo, systemInfo);
        updateMemoryePerformanceLabel(memoryinfo, systemInfo);
        // 创建 QTabWidget
        QTabWidget* tabWidget = new QTabWidget(this);
        cpuInfoLabel = new QLabel("CPU Usage: 0%", this);
      //  mainLayout->addWidget(cpuInfoLabel);

        // 添加 QChartView 和图表
        cpuChart = new QChart;
        cpuSeries = new QLineSeries(this);
        cpuChartView = new QChartView(cpuChart);

        setupChart(cpuChartView, cpuSeries, "CPU Usage");

      //  mainLayout->addWidget(cpuChartView);

        // 添加 QLabel 用于显示内存信息
        memoryInfoLabel = new QLabel("Memory Usage: 0%", this);
      //  mainLayout->addWidget(memoryInfoLabel);

        // 添加 QChartView 和图表
        memoryChart = new QChart;
        memorySeries = new QLineSeries(this);
        memoryChartView = new QChartView(memoryChart);

        setupChart(memoryChartView, memorySeries, "Memory Usage");

       // mainLayout->addWidget(memoryChartView);

        // 添加 QLabel 用于显示网络信息
        networkInfoLabel = new QLabel("Network Upload Speed: 0", this);
      //  mainLayout->addWidget(networkInfoLabel);

        // 添加 QChartView 和图表
        networkChart = new QChart;
        uploadSeries = new QLineSeries(this);
        downloadSeries = new QLineSeries(this);
        networkChartView = new QChartView(networkChart);

        // 创建三个选项卡
        QWidget* memoryTab = createTab("Memory Usage", memoryInfoLabel, memoryChartView, memorySeries, memoryinfo);
        QWidget* cpuTab = createTab("CPU Usage", cpuInfoLabel, cpuChartView, cpuSeries, cpuinfo);
        QWidget* networkTab = createNetTab("Network Usage", networkInfoLabel, networkChartView, uploadSeries, downloadSeries);

        // 将选项卡添加到 QTabWidget
        tabWidget->addTab(cpuTab, "CPU");
        tabWidget->addTab(memoryTab, "Memory");
        tabWidget->addTab(networkTab, "Network");

        // 将选项卡靠左边显示
        tabWidget->setTabPosition(QTabWidget::West);
        tabWidget->setStyleSheet("QTabBar::tab { width: 50px; height: 110px; }");

        // 将 QTabWidget 添加到主布局
        mainLayout->addWidget(tabWidget);
      
        // 创建 SystemMonitor 实例
        monitor = new SystemMonitor();

        // 创建一个单独的线程
        monitorThread = new QThread(this);

        // 将 SystemMonitor 移动到新线程中
        monitor->moveToThread(monitorThread);

        // 连接 SystemMonitor 的信号到更新槽函数
        connect(monitor, &SystemMonitor::cpuUsageUpdated, this, &SystemMonitorWidget::updateCpuUsage);
        connect(monitor, &SystemMonitor::memoryUsageUpdated, this, &SystemMonitorWidget::updateMemoryUsage);
        connect(monitor, &SystemMonitor::networkUsageUpdated, this, &SystemMonitorWidget::updateNetworkUsage);
        connect(monitorThread, &QThread::started, monitor, &SystemMonitor::startMonitoring);

        // 启动线程
        monitorThread->start();

        // 设置主布局
        setLayout(mainLayout);
    }
    ~SystemMonitorWidget()
    {
        // 在程序结束时，停止线程
        monitorThread->quit();
        monitorThread->wait();
    }

public slots:
    void updateSelectedNetworkAdapter(int index)
    {
        if (index >= 0 && index < networkAdapterDescriptions.size())
        {
            QString selectedNetworkAdapter = networkAdapterDescriptions.at(index);
            // 在这里执行处理选定网卡的逻辑，可以更新图表数据等
            // 例如，可以将选定的网卡描述传递给 SystemMonitor 实例，然后更新网络数据
           // monitor->setSelectedNetworkAdapter(selectedNetworkAdapter);
            monitor->setSelectedNetworkAdapter(selectedNetworkAdapter);
            clearChartSeriesData(uploadSeries);
            clearChartSeriesData(downloadSeries);
        }
    }

    // 在 SystemMonitor 类中添加一个公共函数用于设置选定的网卡

    void updateCpuUsage(double cpuUsage)
    {
        // 更新 QLabel 显示 CPU 信息
        cpuInfoLabel->setText(QString("CPU Usage: %1%").arg(cpuUsage+3));

        // 更新图表数据
        appendData(cpuChartView, cpuSeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), (cpuUsage+3));
    }

    void updateMemoryUsage(double usedMemory, double totalMemory)
    {
        // 计算内存使用率
        double memoryUsage = (usedMemory / totalMemory) * 100;

        // 更新 QLabel 显示内存信息
        memoryInfoLabel->setText(QString("Memory Usage: %1% windows:mem total:%2MB,use:%3MB,").arg(memoryUsage, 0, 'f', 2).arg(QString::number(totalMemory, 'f', 2)).arg(QString::number(usedMemory, 'f', 2))
        );

        // 更新图表数据
        appendData(memoryChartView, memorySeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), memoryUsage );
    }

    void updateNetworkUsage(double uploadSpeed, double downloadSpeed)
    {
        // 更新 QLabel 显示网络信息
        networkInfoLabel->setText(QString("Network Upload Speed: %1kbps, Download Speed: %2kbps")
            .arg(uploadSpeed)
            .arg(downloadSpeed));

        // 更新图表数据
        appendData(networkChartView, uploadSeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), uploadSpeed);
        appendData(networkChartView, downloadSeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), downloadSpeed);
    }

private:
    QLabel* cpuInfoLabel;
    QLabel* memoryInfoLabel;
    QLabel* networkInfoLabel;

    QChart* cpuChart;
    QLineSeries* cpuSeries;
    QChartView* cpuChartView;

    QChart* memoryChart;
    QLineSeries* memorySeries;
    QChartView* memoryChartView;

    QChart* networkChart;
    QLineSeries* uploadSeries;
    QLineSeries* downloadSeries;
    QChartView* networkChartView;

    SystemMonitor* monitor;
    QThread* monitorThread;

    QStringList networkAdapterDescriptions;
    QStringList getNetworkAdapterDescriptions() {
        QStringList adapterDescriptions;

        // 获取适配器信息
        IP_ADAPTER_INFO* pAdapterInfo = nullptr;
        ULONG ulBufferSize = 0;

        if (GetAdaptersInfo(nullptr, &ulBufferSize) == ERROR_BUFFER_OVERFLOW) {
            pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(ulBufferSize));

            if (pAdapterInfo) {
                if (GetAdaptersInfo(pAdapterInfo, &ulBufferSize) == NO_ERROR) {
                    // 遍历适配器信息
                    IP_ADAPTER_INFO* pAdapter = pAdapterInfo;
                    while (pAdapter) {
                        // 检查描述中是否包含'#'
                        QString description = QString::fromStdString(pAdapter->Description);
                        if (!description.contains("#")) {
                            adapterDescriptions.append(description);
                        }

                        pAdapter = pAdapter->Next;
                    }
                }

                free(pAdapterInfo);
            }
        }

        return adapterDescriptions;
    }
    void setupChart(QChartView* chartView, QLineSeries* series, const QString& title)
    {
        // 创建默认的坐标轴
        QDateTimeAxis* axisX = new QDateTimeAxis;

        // 设置坐标轴范围，根据您的数据范围来调整
        axisX->setFormat("hh:mm:ss");
        axisX->setTickCount(10);


        QValueAxis* axisY = new QValueAxis;
        // 设置y轴范围
        axisY->setRange(0, 100);

        // 设置y轴标签格式和标题
        axisY->setLabelFormat("%d%");
        axisY->setTitleText("Usage (%)");
        // 将坐标轴添加到图表
        chartView->chart()->addSeries(series);

        // 设置横坐标轴和纵坐标轴
        chartView->chart()->setAxisX(axisX, series);
        chartView->chart()->setAxisY(axisY, series);

        chartView->chart()->setTitle(title);

        // 设置图表视图
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    }

    void appendData(QChartView* chartView, QLineSeries* series, qint64 timestamp, double value)
    {
        // 在数据末尾追加新的数据点
        series->append(timestamp, value);

        // 根据数据量调整横坐标轴范围
        QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
        if (axisX)
        {
            qint64 maxX = series->points().isEmpty() ? timestamp : series->points().last().x();
            qint64 minX = maxX - 60000;  // 60秒范围，可以根据需要调整

            // 设置横坐标轴范围
            axisX->setMin(QDateTime::fromMSecsSinceEpoch(minX));
            axisX->setMax(QDateTime::fromMSecsSinceEpoch(maxX));
        }
    }
    QWidget* createTab(const QString& title, QLabel* infoLabel, QChartView* chartView, QLineSeries* series,QLabel* otherInfoLabel)
    {
        QWidget* tab = new QWidget(this);

        QVBoxLayout* tabLayout = new QVBoxLayout(tab);

        tabLayout->addWidget(infoLabel);
        tabLayout->addWidget(chartView);
        tabLayout->addWidget(otherInfoLabel);
        tab->setLayout(tabLayout);

        // 创建 Chart，并设置到 ChartView
        QChart* chart = new QChart;
        setupChart(chartView, series, title);

        return tab;
    }
    QWidget* createNetTab(const QString& title, QLabel* infoLabel, QChartView* chartView, QLineSeries* series1, QLineSeries* series2)
    {
        networkAdapterDescriptions = getNetworkAdapterDescriptions();

        // 在Network Usage页面上添加一个选择栏
        QComboBox* networkAdapterComboBox = new QComboBox(this);
        networkAdapterComboBox->addItems(networkAdapterDescriptions);
        connect(networkAdapterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SystemMonitorWidget::updateSelectedNetworkAdapter);

        QWidget* tab = new QWidget(this);

        QVBoxLayout* tabLayout = new QVBoxLayout(tab);
        tabLayout->addWidget(networkAdapterComboBox);
        tabLayout->addWidget(infoLabel);
        tabLayout->addWidget(chartView);

        tab->setLayout(tabLayout);

        // 创建 Chart，并设置到 ChartView
        QChart* chart = new QChart;
        setupNetChart(chartView, series1, series2, title);

        return tab;
    }
    void setupNetChart(QChartView* chartView, QLineSeries* series1, QLineSeries* series2, const QString& title)
    {
        // 创建默认的坐标轴
        QDateTimeAxis* axisX = new QDateTimeAxis;

        // 设置坐标轴范围，根据您的数据范围来调整
        axisX->setFormat("hh:mm:ss");
        axisX->setTickCount(10);

        QValueAxis* axisY = new QValueAxis;
        axisY->setRange(0, 100);
        // 设置y轴标签格式和标题
        axisY->setLabelFormat("%dkbps");
        axisY->setTitleText(" Throughput (kbps)");
        // 将坐标轴添加到图表
        chartView->chart()->addSeries(series1);
        chartView->chart()->addSeries(series2);

        // 设置横坐标轴和纵坐标轴
        chartView->chart()->setAxisX(axisX, series1);
        chartView->chart()->setAxisY(axisY, series1);
        chartView->chart()->setAxisX(axisX, series2);
        chartView->chart()->setAxisY(axisY, series2);

        chartView->chart()->setTitle(title);

        // 设置图表视图
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    }
    void updateCpuPerformanceLabel(QLabel* label, const SYSTEM_INFO& systemInfo)
    {
        QString info;
        QTextStream stream(&info);
        stream << QStringLiteral("处理器掩码:") << systemInfo.dwActiveProcessorMask << '\n';
        stream << QStringLiteral("处理器个数:") << systemInfo.dwNumberOfProcessors << '\n';
        stream << QStringLiteral("处理器分页大小:") << systemInfo.dwPageSize << '\n';
        stream << QStringLiteral("处理器类型:") << systemInfo.dwProcessorType << '\n';
        stream << QStringLiteral("最大寻址单元:") << systemInfo.lpMaximumApplicationAddress << '\n';
        stream << QStringLiteral("最小寻址单元:") << systemInfo.lpMinimumApplicationAddress << '\n';
        stream << QStringLiteral("处理器等级:") << systemInfo.wProcessorLevel << '\n';
        stream << QStringLiteral("处理器版本:") << systemInfo.wProcessorRevision << '\n';
        label->setText(info);
    }
    void updateMemoryePerformanceLabel(QLabel* label, const SYSTEM_INFO& systemInfo)
    {
        QString info;
        QTextStream stream(&info);
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        stream << QStringLiteral("物理内存总量:") << statex.ullTotalPhys/(1024*1024) << "MB"<<'\n';
        stream << QStringLiteral("可用的物理内存:") << statex.ullAvailPhys / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("系统页面文件大小:") << statex.ullTotalPageFile / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("系统可用页面文件大小:") << statex.ullAvailPageFile / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("虚拟内存总量:") << statex.ullTotalVirtual / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("可用的虚拟内存:") << statex.ullAvailVirtual / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("保留（值为0）:") << statex.ullAvailExtendedVirtual / (1024 * 1024) << "MB";
        label->setText(info);
    }
    void clearChartSeriesData(QLineSeries* series)
    {
        if (series)
        {
            series->clear();
        }
    }
};

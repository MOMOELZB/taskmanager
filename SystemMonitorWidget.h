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
        // ����������
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        QLabel* cpuinfo = new QLabel();
        QLabel* memoryinfo = new QLabel();
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        updateCpuPerformanceLabel(cpuinfo, systemInfo);
        updateMemoryePerformanceLabel(memoryinfo, systemInfo);
        // ���� QTabWidget
        QTabWidget* tabWidget = new QTabWidget(this);
        cpuInfoLabel = new QLabel("CPU Usage: 0%", this);
      //  mainLayout->addWidget(cpuInfoLabel);

        // ��� QChartView ��ͼ��
        cpuChart = new QChart;
        cpuSeries = new QLineSeries(this);
        cpuChartView = new QChartView(cpuChart);

        setupChart(cpuChartView, cpuSeries, "CPU Usage");

      //  mainLayout->addWidget(cpuChartView);

        // ��� QLabel ������ʾ�ڴ���Ϣ
        memoryInfoLabel = new QLabel("Memory Usage: 0%", this);
      //  mainLayout->addWidget(memoryInfoLabel);

        // ��� QChartView ��ͼ��
        memoryChart = new QChart;
        memorySeries = new QLineSeries(this);
        memoryChartView = new QChartView(memoryChart);

        setupChart(memoryChartView, memorySeries, "Memory Usage");

       // mainLayout->addWidget(memoryChartView);

        // ��� QLabel ������ʾ������Ϣ
        networkInfoLabel = new QLabel("Network Upload Speed: 0", this);
      //  mainLayout->addWidget(networkInfoLabel);

        // ��� QChartView ��ͼ��
        networkChart = new QChart;
        uploadSeries = new QLineSeries(this);
        downloadSeries = new QLineSeries(this);
        networkChartView = new QChartView(networkChart);

        // ��������ѡ�
        QWidget* memoryTab = createTab("Memory Usage", memoryInfoLabel, memoryChartView, memorySeries, memoryinfo);
        QWidget* cpuTab = createTab("CPU Usage", cpuInfoLabel, cpuChartView, cpuSeries, cpuinfo);
        QWidget* networkTab = createNetTab("Network Usage", networkInfoLabel, networkChartView, uploadSeries, downloadSeries);

        // ��ѡ���ӵ� QTabWidget
        tabWidget->addTab(cpuTab, "CPU");
        tabWidget->addTab(memoryTab, "Memory");
        tabWidget->addTab(networkTab, "Network");

        // ��ѡ��������ʾ
        tabWidget->setTabPosition(QTabWidget::West);
        tabWidget->setStyleSheet("QTabBar::tab { width: 50px; height: 110px; }");

        // �� QTabWidget ��ӵ�������
        mainLayout->addWidget(tabWidget);
      
        // ���� SystemMonitor ʵ��
        monitor = new SystemMonitor();

        // ����һ���������߳�
        monitorThread = new QThread(this);

        // �� SystemMonitor �ƶ������߳���
        monitor->moveToThread(monitorThread);

        // ���� SystemMonitor ���źŵ����²ۺ���
        connect(monitor, &SystemMonitor::cpuUsageUpdated, this, &SystemMonitorWidget::updateCpuUsage);
        connect(monitor, &SystemMonitor::memoryUsageUpdated, this, &SystemMonitorWidget::updateMemoryUsage);
        connect(monitor, &SystemMonitor::networkUsageUpdated, this, &SystemMonitorWidget::updateNetworkUsage);
        connect(monitorThread, &QThread::started, monitor, &SystemMonitor::startMonitoring);

        // �����߳�
        monitorThread->start();

        // ����������
        setLayout(mainLayout);
    }
    ~SystemMonitorWidget()
    {
        // �ڳ������ʱ��ֹͣ�߳�
        monitorThread->quit();
        monitorThread->wait();
    }

public slots:
    void updateSelectedNetworkAdapter(int index)
    {
        if (index >= 0 && index < networkAdapterDescriptions.size())
        {
            QString selectedNetworkAdapter = networkAdapterDescriptions.at(index);
            // ������ִ�д���ѡ���������߼������Ը���ͼ�����ݵ�
            // ���磬���Խ�ѡ���������������ݸ� SystemMonitor ʵ����Ȼ�������������
           // monitor->setSelectedNetworkAdapter(selectedNetworkAdapter);
            monitor->setSelectedNetworkAdapter(selectedNetworkAdapter);
            clearChartSeriesData(uploadSeries);
            clearChartSeriesData(downloadSeries);
        }
    }

    // �� SystemMonitor �������һ������������������ѡ��������

    void updateCpuUsage(double cpuUsage)
    {
        // ���� QLabel ��ʾ CPU ��Ϣ
        cpuInfoLabel->setText(QString("CPU Usage: %1%").arg(cpuUsage+3));

        // ����ͼ������
        appendData(cpuChartView, cpuSeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), (cpuUsage+3));
    }

    void updateMemoryUsage(double usedMemory, double totalMemory)
    {
        // �����ڴ�ʹ����
        double memoryUsage = (usedMemory / totalMemory) * 100;

        // ���� QLabel ��ʾ�ڴ���Ϣ
        memoryInfoLabel->setText(QString("Memory Usage: %1% windows:mem total:%2MB,use:%3MB,").arg(memoryUsage, 0, 'f', 2).arg(QString::number(totalMemory, 'f', 2)).arg(QString::number(usedMemory, 'f', 2))
        );

        // ����ͼ������
        appendData(memoryChartView, memorySeries, QDateTime::currentDateTime().toMSecsSinceEpoch(), memoryUsage );
    }

    void updateNetworkUsage(double uploadSpeed, double downloadSpeed)
    {
        // ���� QLabel ��ʾ������Ϣ
        networkInfoLabel->setText(QString("Network Upload Speed: %1kbps, Download Speed: %2kbps")
            .arg(uploadSpeed)
            .arg(downloadSpeed));

        // ����ͼ������
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

        // ��ȡ��������Ϣ
        IP_ADAPTER_INFO* pAdapterInfo = nullptr;
        ULONG ulBufferSize = 0;

        if (GetAdaptersInfo(nullptr, &ulBufferSize) == ERROR_BUFFER_OVERFLOW) {
            pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(malloc(ulBufferSize));

            if (pAdapterInfo) {
                if (GetAdaptersInfo(pAdapterInfo, &ulBufferSize) == NO_ERROR) {
                    // ������������Ϣ
                    IP_ADAPTER_INFO* pAdapter = pAdapterInfo;
                    while (pAdapter) {
                        // ����������Ƿ����'#'
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
        // ����Ĭ�ϵ�������
        QDateTimeAxis* axisX = new QDateTimeAxis;

        // ���������᷶Χ�������������ݷ�Χ������
        axisX->setFormat("hh:mm:ss");
        axisX->setTickCount(10);


        QValueAxis* axisY = new QValueAxis;
        // ����y�᷶Χ
        axisY->setRange(0, 100);

        // ����y���ǩ��ʽ�ͱ���
        axisY->setLabelFormat("%d%");
        axisY->setTitleText("Usage (%)");
        // ����������ӵ�ͼ��
        chartView->chart()->addSeries(series);

        // ���ú����������������
        chartView->chart()->setAxisX(axisX, series);
        chartView->chart()->setAxisY(axisY, series);

        chartView->chart()->setTitle(title);

        // ����ͼ����ͼ
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    }

    void appendData(QChartView* chartView, QLineSeries* series, qint64 timestamp, double value)
    {
        // ������ĩβ׷���µ����ݵ�
        series->append(timestamp, value);

        // ���������������������᷶Χ
        QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
        if (axisX)
        {
            qint64 maxX = series->points().isEmpty() ? timestamp : series->points().last().x();
            qint64 minX = maxX - 60000;  // 60�뷶Χ�����Ը�����Ҫ����

            // ���ú������᷶Χ
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

        // ���� Chart�������õ� ChartView
        QChart* chart = new QChart;
        setupChart(chartView, series, title);

        return tab;
    }
    QWidget* createNetTab(const QString& title, QLabel* infoLabel, QChartView* chartView, QLineSeries* series1, QLineSeries* series2)
    {
        networkAdapterDescriptions = getNetworkAdapterDescriptions();

        // ��Network Usageҳ�������һ��ѡ����
        QComboBox* networkAdapterComboBox = new QComboBox(this);
        networkAdapterComboBox->addItems(networkAdapterDescriptions);
        connect(networkAdapterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SystemMonitorWidget::updateSelectedNetworkAdapter);

        QWidget* tab = new QWidget(this);

        QVBoxLayout* tabLayout = new QVBoxLayout(tab);
        tabLayout->addWidget(networkAdapterComboBox);
        tabLayout->addWidget(infoLabel);
        tabLayout->addWidget(chartView);

        tab->setLayout(tabLayout);

        // ���� Chart�������õ� ChartView
        QChart* chart = new QChart;
        setupNetChart(chartView, series1, series2, title);

        return tab;
    }
    void setupNetChart(QChartView* chartView, QLineSeries* series1, QLineSeries* series2, const QString& title)
    {
        // ����Ĭ�ϵ�������
        QDateTimeAxis* axisX = new QDateTimeAxis;

        // ���������᷶Χ�������������ݷ�Χ������
        axisX->setFormat("hh:mm:ss");
        axisX->setTickCount(10);

        QValueAxis* axisY = new QValueAxis;
        axisY->setRange(0, 100);
        // ����y���ǩ��ʽ�ͱ���
        axisY->setLabelFormat("%dkbps");
        axisY->setTitleText(" Throughput (kbps)");
        // ����������ӵ�ͼ��
        chartView->chart()->addSeries(series1);
        chartView->chart()->addSeries(series2);

        // ���ú����������������
        chartView->chart()->setAxisX(axisX, series1);
        chartView->chart()->setAxisY(axisY, series1);
        chartView->chart()->setAxisX(axisX, series2);
        chartView->chart()->setAxisY(axisY, series2);

        chartView->chart()->setTitle(title);

        // ����ͼ����ͼ
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    }
    void updateCpuPerformanceLabel(QLabel* label, const SYSTEM_INFO& systemInfo)
    {
        QString info;
        QTextStream stream(&info);
        stream << QStringLiteral("����������:") << systemInfo.dwActiveProcessorMask << '\n';
        stream << QStringLiteral("����������:") << systemInfo.dwNumberOfProcessors << '\n';
        stream << QStringLiteral("��������ҳ��С:") << systemInfo.dwPageSize << '\n';
        stream << QStringLiteral("����������:") << systemInfo.dwProcessorType << '\n';
        stream << QStringLiteral("���Ѱַ��Ԫ:") << systemInfo.lpMaximumApplicationAddress << '\n';
        stream << QStringLiteral("��СѰַ��Ԫ:") << systemInfo.lpMinimumApplicationAddress << '\n';
        stream << QStringLiteral("�������ȼ�:") << systemInfo.wProcessorLevel << '\n';
        stream << QStringLiteral("�������汾:") << systemInfo.wProcessorRevision << '\n';
        label->setText(info);
    }
    void updateMemoryePerformanceLabel(QLabel* label, const SYSTEM_INFO& systemInfo)
    {
        QString info;
        QTextStream stream(&info);
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        stream << QStringLiteral("�����ڴ�����:") << statex.ullTotalPhys/(1024*1024) << "MB"<<'\n';
        stream << QStringLiteral("���õ������ڴ�:") << statex.ullAvailPhys / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("ϵͳҳ���ļ���С:") << statex.ullTotalPageFile / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("ϵͳ����ҳ���ļ���С:") << statex.ullAvailPageFile / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("�����ڴ�����:") << statex.ullTotalVirtual / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("���õ������ڴ�:") << statex.ullAvailVirtual / (1024 * 1024) << "MB" << '\n';
        stream << QStringLiteral("������ֵΪ0��:") << statex.ullAvailExtendedVirtual / (1024 * 1024) << "MB";
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

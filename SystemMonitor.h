#include <QtWidgets>
#include <QtCharts>
#include"Monitorutil.h"
#include"mysysinfo.h"
class SystemMonitor : public QObject
{
    Q_OBJECT
public:
    SystemMonitor::SystemMonitor(QObject* parent=nullptr) : QObject(parent)
    {
         monitor = new MySysInfo();
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SystemMonitor::updatePerformanceData);
    }

    void SystemMonitor::startMonitoring()
    {
        timer->start(500); // Update every second, you can adjust this interval
    }

    void SystemMonitor::stopMonitoring()
    {
        timer->stop();
    }
    void setSelectedNetworkAdapter(const QString& adapterDescription)
    {
        Adapter = adapterDescription;
    }
signals:
    void cpuUsageUpdated(double usage);
    void memoryUsageUpdated(double usedMemory, double totalMemory);
    void networkUsageUpdated(double uploadSpeed, double downloadSpeed);

private slots:
    void updatePerformanceData()
    {
        // Obtain and emit CPU, memory, and network usage data
        // Customize this function based on your requirements
        monitor->GetNetUsage(uploadSpeed, downloadSpeed,Adapter);
        monitor->GetCpuUsage(cpuUsage);
        monitor->GetMemUsage(totalMemory, usedMemory);
       
        emit cpuUsageUpdated(cpuUsage);
        emit memoryUsageUpdated(usedMemory, totalMemory);
        emit networkUsageUpdated(uploadSpeed, downloadSpeed);
    }
private:
    QTimer* timer;
    MySysInfo* monitor ;
    QString Adapter;
    double usedMemory = 0;
    double cpuUsage = 0;
    double totalMemory = 0;
    double uploadSpeed = 0;
    double downloadSpeed = 0;
};


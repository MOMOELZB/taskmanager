#pragma once
#include <QThread>
#include <QDialog>
#include <QVBoxLayout>
#include <QTimer>
#ifndef PROCESSDETAILSTHREAD_H
#define PROCESSDETAILSTHREAD_H
#include <QObject>
#include <windows.h>
#include <tlhelp32.h>
#include <QGridLayout>
#include <QLabel>
#include <Psapi.h>
#include<qdebug.h>
#include "mysysinfo.h"
#include<qpushbutton.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qpropertyanimation.h>
#include <qstatemachine.h>
#include <Windows.h>
#include <Wlanapi.h>  
#include <bluetoothapis.h>
#include <QPushButton>
#include <QSlider>
#include "screen.h"
#include "volume.h"
#include <QFileDialog>
#include <QDesktopServices>

class ProcessMonitorThread : public QThread
{
    Q_OBJECT

public:
    explicit ProcessMonitorThread(QObject* parent = nullptr);
    void run() override;

signals:
    void processListChanged(const QList<PROCESSENTRY32>& newProcessList);

private:
    QList<QString> getProcessListHash();
    QString hashProcessEntry(const PROCESSENTRY32& processEntry);

    QList<QString> lastProcessListHash;
};
#endif 
class WorkerThread : public QThread
{
    Q_OBJECT
public:
    WorkerThread(DWORD processID, QObject* parent = nullptr) : QThread(parent), processID(processID) {}

protected:
    void run() override
    {
        // 在这里执行结束进程的操作
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }

        // 当操作完成后，发出信号通知主线程
        emit operationCompleted();
    }

signals:
    void operationCompleted();

private:
    DWORD processID;
};
class CalculationThread : public QThread
{
    Q_OBJECT

public:
    CalculationThread(QObject* parent = nullptr) : QThread(parent)
    {
        monitor = new MySysInfo();
        lastSystemTime = getCurrentSystemTime();  // 初始化为当前系统时间
        lastProcessTime = getCurrentProcessTime();  // 初始化为当前进程时间
    }
 ~CalculationThread()
    {
     quit();
     wait();
    }
    void setProcessID(DWORD newProcessID)
    {
        processID = newProcessID;
    }

  

signals:
    void calculationCompleted(double cpuPercentage, double memoryUsageMB);

protected:
    void run() override
    {
        while (!isInterruptionRequested()) {
            calculateInfo();
            msleep(1000);
        }
       
    }

private slots:
    void calculateInfo()
    {
        if (processID == -1)
        {
            monitor->GetCpuUsage(cpuPercentage);
            monitor->GetMemUsage(totalmemory, memoryUsageMB);
            emit calculationCompleted(cpuPercentage, memoryUsageMB);
        }
        else {
            qDebug() << processID;
            FILETIME idleTime, kernelTime, userTime;

            if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
            {
                ULONGLONG totalSystemTime = (static_cast<ULONGLONG>(kernelTime.dwHighDateTime) << 32) + kernelTime.dwLowDateTime +
                    (static_cast<ULONGLONG>(userTime.dwHighDateTime) << 32) + userTime.dwLowDateTime;

                if (totalSystemTime - lastSystemTime>0)
                {
                    ULONGLONG systemTimeDelta = totalSystemTime - lastSystemTime;

                    // 获取进程时间信息
                    FILETIME createTime, exitTime, processKernelTime, processUserTime;
                    GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &processKernelTime, &processUserTime);

                    ULONGLONG processTime = (static_cast<ULONGLONG>(processKernelTime.dwHighDateTime) << 32) + processKernelTime.dwLowDateTime +
                        (static_cast<ULONGLONG>(processUserTime.dwHighDateTime) << 32) + processUserTime.dwLowDateTime;

                    ULONGLONG processTimeDelta = processTime - lastProcessTime;

                    // 计算 CPU 使用率
                  cpuPercentage = static_cast<double>(processTimeDelta) / systemTimeDelta * 100.0;
                   // cpuLabel->setText(QString("CPU 使用率: %1%").arg(cpuPercentage, 0, 'f', 2));

                    // 获取内存信息
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
                    {
                        // 转换为 MB
                        memoryUsageMB = static_cast<double>(pmc.PrivateUsage) / (1024 * 1024);
                       // memoryLabel->setText(QString("内存 使用量: %1 MB").arg(memoryUsageMB, 0, 'f', 2));
                    }
                    emit calculationCompleted(cpuPercentage, memoryUsageMB);
                    lastSystemTime = totalSystemTime;
                    lastProcessTime = processTime;
                }
            }
            
            // 执行计算操作，并通过信号发送结果
        }

       
    }
    ULONGLONG getCurrentSystemTime()
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
    }

    ULONGLONG getCurrentProcessTime()
    {
        FILETIME createTime, exitTime, kernelTime, userTime;
        GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime);
        return (static_cast<ULONGLONG>(kernelTime.dwHighDateTime) << 32) + kernelTime.dwLowDateTime +
            (static_cast<ULONGLONG>(userTime.dwHighDateTime) << 32) + userTime.dwLowDateTime;
    }
private:
    double totalmemory = 0;
    double memoryUsageMB = 0;
    double cpuPercentage = 0;
    MySysInfo* monitor;
    QTimer* timer;
    DWORD processID=-1;
    ULONGLONG lastSystemTime = 0;
    ULONGLONG lastProcessTime = 0;
};
class AdditionalInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdditionalInfoWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
       
        QVBoxLayout* layout = new QVBoxLayout(this);
        volume = Volume();
        screen = Screen();
        // 创建一个新的 QWidget 用于容纳 QLabel 和 QPushButton
       
        QVBoxLayout* containerLayout = new QVBoxLayout();

        InfoLabel = new QLabel("Total CPU:");
        EndProcess = new QPushButton("End Process");

        // 将 QLabel 和 QPushButton 添加到容器布局中
        containerLayout->addWidget(InfoLabel);
        containerLayout->addWidget(EndProcess);

        // 将容器 QWidget 添加到主布局中
        layout->addLayout(containerLayout);
        // Add Wi-Fi, Bluetooth, and Flight Mode switches
   // Add animated switches
        wifiSwitch = createAnimatedSwitch("Wi-Fi");
        bluetoothSwitch = createAnimatedSwitch("Bluetooth");
        flightModeSwitch = createAnimatedSwitch("New Process");

        // 创建水平布局并将按钮添加到布局中
       
        QVBoxLayout* toolLayout = new QVBoxLayout();
        QHBoxLayout* switchLayout = new QHBoxLayout();
        switchLayout->addWidget(wifiSwitch);
        switchLayout->addWidget(bluetoothSwitch);
        switchLayout->addWidget(flightModeSwitch);

        // 将水平布局添加到主布局中
        toolLayout->addLayout(switchLayout);

        // 创建音量部分
        QHBoxLayout* volumeLayout = new QHBoxLayout();
        QLabel* volumeLabel = new QLabel("Volume:");
        volumeLayout->addWidget(volumeLabel);

        volumeSlider = new QSlider(Qt::Horizontal);
        volumeLayout->addWidget(volumeSlider);

        muteButton = new QPushButton("Mute");
        volumeLayout->addWidget(muteButton);

        // 添加音量部分到主布局
        toolLayout->addLayout(volumeLayout);

        // 创建亮度部分
        QHBoxLayout* brightnessLayout = new QHBoxLayout();
        QLabel* brightnessLabel = new QLabel("Brightness:");
        brightnessLayout->addWidget(brightnessLabel);

        brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessLayout->addWidget(brightnessSlider);

        // 添加亮度部分到主布局
        toolLayout->addLayout(brightnessLayout);
        layout->addLayout(toolLayout);
        calculationThread = new CalculationThread(this);
        connect(volumeSlider, &QSlider::valueChanged, this, &AdditionalInfoWidget::onVolumeChanged);
        connect(brightnessSlider, &QSlider::valueChanged, this, &AdditionalInfoWidget::onBrightnessChanged);
        connect(muteButton, &QPushButton::clicked, this, &AdditionalInfoWidget::onMuteClicked);
        // 连接按钮点击事件到槽函数
        connect(EndProcess, &QPushButton::clicked, this, &AdditionalInfoWidget::endProcessClicked);
        connect(calculationThread, &CalculationThread::calculationCompleted, this, &AdditionalInfoWidget::updateInfo);
        connect(wifiSwitch, &QToolButton::clicked, this, &AdditionalInfoWidget::toggleWifi);
        connect(bluetoothSwitch, &QToolButton::clicked, this, &AdditionalInfoWidget::toggleBluetooth);
        connect(flightModeSwitch, &QToolButton::clicked, this, &AdditionalInfoWidget::toggleFlightMode);
        calculationThread->start();
    }
    QToolButton* createAnimatedSwitch(const QString& text)
    {
        QToolButton* switchButton = new QToolButton();
        switchButton->setText(text);
        switchButton->setCheckable(true);
        switchButton->setFixedSize(100, 40);
        QStateMachine* stateMachine = new QStateMachine(this);

        QState* onState = new QState();
        QState* offState = new QState();

        stateMachine->addState(onState);
        stateMachine->addState(offState);
        stateMachine->setInitialState(offState);

        QPropertyAnimation* animation = new QPropertyAnimation(switchButton, "iconSize");
        animation->setDuration(300);
        animation->setStartValue(QSize(40, 40));
        animation->setEndValue(QSize(20, 20));

        onState->assignProperty(switchButton, "checked", true);
        onState->addTransition(switchButton, &QToolButton::clicked, offState);
        onState->addTransition(animation, &QPropertyAnimation::finished, offState);

        offState->assignProperty(switchButton, "checked", false);
        offState->addTransition(switchButton, &QToolButton::clicked, onState);
        offState->addTransition(animation, &QPropertyAnimation::finished, onState);

        connect(stateMachine, &QStateMachine::started, [animation]() {
            animation->start();
            });

        stateMachine->start();

        return switchButton;
    }
public slots:
    void changupdateInfo(DWORD processid,QString name)
    {
        calculationThread->setProcessID(processid);
        processName = name;
        processId = processid;
       // openFileManagerByPid(processId);

    }
   

    void AdditionalInfoWidget::onVolumeChanged(int value)
    {
        qDebug() << "Volume changed:" << value;
        volume.SetSystemVolume(value);
        // 处理音量滑块值变化的操作
        
    }

    void AdditionalInfoWidget::onBrightnessChanged(int value)
    {

        screen.SetScreenBrightness(value);
        // 处理亮度滑块值变化的操作
        qDebug() << "Brightness changed:" << value;
    }

    void AdditionalInfoWidget::onMuteClicked()
    {
        // 处理静音按钮点击的操作
        if (flagVolume == 1)
        {
            volume.SetSystemVolume(-1);
            flagVolume = 0;
        }
        else
        {
            volume.SetSystemVolume(-2);
            flagVolume = 1;
        }
    }
    void AdditionalInfoWidget::toggleWifi()
    {
        QString wifiInterfaceName = "WLAN";  // 替换为正确的 Wi-Fi 接口名称

        
        
        if (wifiSwitch->isChecked()) {
            // Wi-Fi 开启时的操作
            qDebug() << "Wi-Fi turned ON";
            QProcess process;
            process.start("netsh wlan connect name = CUMTB-EDU");
            process.waitForFinished();
        }
        else {
            // Wi-Fi 关闭时的操作
            qDebug() << "Wi-Fi turned OFF";
            QProcess process;
            process.start("netsh wlan disconnect");
            process.waitForFinished();

            // 输出错误信息
            qDebug() << "Error: " << process.readAllStandardError();
        }
    }


    void toggleBluetooth()
    {
        // 使用 SystemParametersInfo 设置系统参数
        const wchar_t* cmd = L"C:\\Windows\\System32\\bthprops.cpl";
        const wchar_t* params = L"";

        // 打开蓝牙设置面板
        ShellExecute(nullptr, L"open", cmd, params, nullptr, SW_SHOWNORMAL);
        // 处理蓝牙开关状态
        if (bluetoothSwitch->isChecked()) {
            // 蓝牙开启时的操作
            qDebug() << "Bluetooth turned ON";
        }
        else {

            // 蓝牙关闭时的操作
            qDebug() << "Bluetooth turned OFF";
        }
    }

    void toggleFlightMode()
    {
        flightModeSwitch->setCheckable(false);        // 打开文件管理器对话框
        QString filePath = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath());

        if (!filePath.isEmpty()) {
            // 运行所选文件
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));

            // 如果是目录，您可以使用下面的代码
            // QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).dir().absolutePath()));
        }
    }

private:
    void toggleSystemSetting(int setting)
    {
        // 使用 SystemParametersInfo 设置系统参数
        BOOL result = SystemParametersInfo(setting, 0, nullptr, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

        if (!result) {
            qDebug() << "Failed to toggle system setting. Error code: " << GetLastError();
        }
    }

    void updateInfo(double cpuPercentage, double memoryUsageMB)
    {  
          InfoLabel->setText(QString("Process name :")+processName+"\n"+
          QString("CPU Using: %1%").arg(cpuPercentage+5, 0, 'f', 2)+"\n" + QString("Memory Using: %1 MB").arg(memoryUsageMB, 0, 'f', 2));
    }
    void endProcessClicked()
    {
        // 结束进程的代码
        if (processId != -1)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
            if (hProcess)
            {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
       
    }
private:
    double totalmemory = 0;
    DWORD processId = -1;
    QLabel* InfoLabel;
    QPushButton* EndProcess;
    QString processName = "Total Process";
    CalculationThread* calculationThread;
    QToolButton* wifiSwitch;
    QToolButton* bluetoothSwitch;
    QToolButton* flightModeSwitch;
    QSlider* volumeSlider;      // 音量滑块
    QSlider* brightnessSlider;  // 亮度滑块
    QPushButton* muteButton;    // 静音按钮
    int flagVolume;
    Volume volume;
    Screen  screen;
};




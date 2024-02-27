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
        // ������ִ�н������̵Ĳ���
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }

        // ��������ɺ󣬷����ź�֪ͨ���߳�
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
        lastSystemTime = getCurrentSystemTime();  // ��ʼ��Ϊ��ǰϵͳʱ��
        lastProcessTime = getCurrentProcessTime();  // ��ʼ��Ϊ��ǰ����ʱ��
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

                    // ��ȡ����ʱ����Ϣ
                    FILETIME createTime, exitTime, processKernelTime, processUserTime;
                    GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &processKernelTime, &processUserTime);

                    ULONGLONG processTime = (static_cast<ULONGLONG>(processKernelTime.dwHighDateTime) << 32) + processKernelTime.dwLowDateTime +
                        (static_cast<ULONGLONG>(processUserTime.dwHighDateTime) << 32) + processUserTime.dwLowDateTime;

                    ULONGLONG processTimeDelta = processTime - lastProcessTime;

                    // ���� CPU ʹ����
                  cpuPercentage = static_cast<double>(processTimeDelta) / systemTimeDelta * 100.0;
                   // cpuLabel->setText(QString("CPU ʹ����: %1%").arg(cpuPercentage, 0, 'f', 2));

                    // ��ȡ�ڴ���Ϣ
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
                    {
                        // ת��Ϊ MB
                        memoryUsageMB = static_cast<double>(pmc.PrivateUsage) / (1024 * 1024);
                       // memoryLabel->setText(QString("�ڴ� ʹ����: %1 MB").arg(memoryUsageMB, 0, 'f', 2));
                    }
                    emit calculationCompleted(cpuPercentage, memoryUsageMB);
                    lastSystemTime = totalSystemTime;
                    lastProcessTime = processTime;
                }
            }
            
            // ִ�м����������ͨ���źŷ��ͽ��
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
        // ����һ���µ� QWidget �������� QLabel �� QPushButton
       
        QVBoxLayout* containerLayout = new QVBoxLayout();

        InfoLabel = new QLabel("Total CPU:");
        EndProcess = new QPushButton("End Process");

        // �� QLabel �� QPushButton ��ӵ�����������
        containerLayout->addWidget(InfoLabel);
        containerLayout->addWidget(EndProcess);

        // ������ QWidget ��ӵ���������
        layout->addLayout(containerLayout);
        // Add Wi-Fi, Bluetooth, and Flight Mode switches
   // Add animated switches
        wifiSwitch = createAnimatedSwitch("Wi-Fi");
        bluetoothSwitch = createAnimatedSwitch("Bluetooth");
        flightModeSwitch = createAnimatedSwitch("New Process");

        // ����ˮƽ���ֲ�����ť��ӵ�������
       
        QVBoxLayout* toolLayout = new QVBoxLayout();
        QHBoxLayout* switchLayout = new QHBoxLayout();
        switchLayout->addWidget(wifiSwitch);
        switchLayout->addWidget(bluetoothSwitch);
        switchLayout->addWidget(flightModeSwitch);

        // ��ˮƽ������ӵ���������
        toolLayout->addLayout(switchLayout);

        // ������������
        QHBoxLayout* volumeLayout = new QHBoxLayout();
        QLabel* volumeLabel = new QLabel("Volume:");
        volumeLayout->addWidget(volumeLabel);

        volumeSlider = new QSlider(Qt::Horizontal);
        volumeLayout->addWidget(volumeSlider);

        muteButton = new QPushButton("Mute");
        volumeLayout->addWidget(muteButton);

        // ����������ֵ�������
        toolLayout->addLayout(volumeLayout);

        // �������Ȳ���
        QHBoxLayout* brightnessLayout = new QHBoxLayout();
        QLabel* brightnessLabel = new QLabel("Brightness:");
        brightnessLayout->addWidget(brightnessLabel);

        brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessLayout->addWidget(brightnessSlider);

        // ������Ȳ��ֵ�������
        toolLayout->addLayout(brightnessLayout);
        layout->addLayout(toolLayout);
        calculationThread = new CalculationThread(this);
        connect(volumeSlider, &QSlider::valueChanged, this, &AdditionalInfoWidget::onVolumeChanged);
        connect(brightnessSlider, &QSlider::valueChanged, this, &AdditionalInfoWidget::onBrightnessChanged);
        connect(muteButton, &QPushButton::clicked, this, &AdditionalInfoWidget::onMuteClicked);
        // ���Ӱ�ť����¼����ۺ���
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
        // ������������ֵ�仯�Ĳ���
        
    }

    void AdditionalInfoWidget::onBrightnessChanged(int value)
    {

        screen.SetScreenBrightness(value);
        // �������Ȼ���ֵ�仯�Ĳ���
        qDebug() << "Brightness changed:" << value;
    }

    void AdditionalInfoWidget::onMuteClicked()
    {
        // ��������ť����Ĳ���
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
        QString wifiInterfaceName = "WLAN";  // �滻Ϊ��ȷ�� Wi-Fi �ӿ�����

        
        
        if (wifiSwitch->isChecked()) {
            // Wi-Fi ����ʱ�Ĳ���
            qDebug() << "Wi-Fi turned ON";
            QProcess process;
            process.start("netsh wlan connect name = CUMTB-EDU");
            process.waitForFinished();
        }
        else {
            // Wi-Fi �ر�ʱ�Ĳ���
            qDebug() << "Wi-Fi turned OFF";
            QProcess process;
            process.start("netsh wlan disconnect");
            process.waitForFinished();

            // ���������Ϣ
            qDebug() << "Error: " << process.readAllStandardError();
        }
    }


    void toggleBluetooth()
    {
        // ʹ�� SystemParametersInfo ����ϵͳ����
        const wchar_t* cmd = L"C:\\Windows\\System32\\bthprops.cpl";
        const wchar_t* params = L"";

        // �������������
        ShellExecute(nullptr, L"open", cmd, params, nullptr, SW_SHOWNORMAL);
        // ������������״̬
        if (bluetoothSwitch->isChecked()) {
            // ��������ʱ�Ĳ���
            qDebug() << "Bluetooth turned ON";
        }
        else {

            // �����ر�ʱ�Ĳ���
            qDebug() << "Bluetooth turned OFF";
        }
    }

    void toggleFlightMode()
    {
        flightModeSwitch->setCheckable(false);        // ���ļ��������Ի���
        QString filePath = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath());

        if (!filePath.isEmpty()) {
            // ������ѡ�ļ�
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));

            // �����Ŀ¼��������ʹ������Ĵ���
            // QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).dir().absolutePath()));
        }
    }

private:
    void toggleSystemSetting(int setting)
    {
        // ʹ�� SystemParametersInfo ����ϵͳ����
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
        // �������̵Ĵ���
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
    QSlider* volumeSlider;      // ��������
    QSlider* brightnessSlider;  // ���Ȼ���
    QPushButton* muteButton;    // ������ť
    int flagVolume;
    Volume volume;
    Screen  screen;
};




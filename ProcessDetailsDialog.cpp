
#include "ProcessInfoThread.h"
#include <windows.h>
#include <windows.h>
#include<qdebug.h>



ProcessMonitorThread::ProcessMonitorThread(QObject* parent)
    : QThread(parent)
{
}
void ProcessMonitorThread::run()
{
    while (!isInterruptionRequested())
    {
        QList<QString> currentProcessListHash = getProcessListHash();

        if (currentProcessListHash != lastProcessListHash)
        {
            lastProcessListHash = currentProcessListHash;

            QList<PROCESSENTRY32> currentProcessList;
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot != INVALID_HANDLE_VALUE)
            {
                PROCESSENTRY32 processEntry;
                processEntry.dwSize = sizeof(PROCESSENTRY32);
                if (Process32First(hSnapshot, &processEntry))
                {
                    do
                    {
                        currentProcessList.append(processEntry);
                    } while (Process32Next(hSnapshot, &processEntry));
                }

                CloseHandle(hSnapshot);
            }

            emit processListChanged(currentProcessList);
        }

        msleep(1000); // ÿ������һ�Σ�Ȼ���ٴμ��
    }
}
QList<QString> ProcessMonitorThread::getProcessListHash()
{
    QList<QString> processListHash;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                processListHash.append(hashProcessEntry(processEntry));
            } while (Process32Next(hSnapshot, &processEntry));
        }

        CloseHandle(hSnapshot);
    }

    return processListHash;
}
QString ProcessMonitorThread::hashProcessEntry(const PROCESSENTRY32& processEntry)
{
    // ����ʵ����Ҫѡ����ʵ��ֶ������ɹ�ϣֵ
    return QString("%1%2").arg(processEntry.th32ProcessID).arg(QString::fromWCharArray(processEntry.szExeFile));
}
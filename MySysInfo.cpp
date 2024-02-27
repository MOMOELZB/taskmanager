#include "mysysinfo.h"
MySysInfo::MySysInfo(QObject* parent) : QObject(parent)
{
    initialize();
}
bool MySysInfo::GetMemUsage(double& nMemTotal, double& nMemUsed)
{

    MEMORYSTATUSEX memsStat;
    memsStat.dwLength = sizeof(memsStat);
    if (!GlobalMemoryStatusEx(&memsStat))//如果获取系统内存信息不成功，就直接返回
    {
        return false;
    }
    double nMemFree = memsStat.ullAvailPhys / (1024.0 * 1024.0);
    nMemTotal = memsStat.ullTotalPhys / (1024.0 * 1024.0);
    nMemUsed = nMemTotal - nMemFree;
    qDebug("windows:mem total:%.0lfMB,use:%.0lfMB", nMemTotal, nMemUsed);
    return true;

}
bool MySysInfo::GetNetUsage(double& uploadSpeed, double& downloadSpeed,QString  Adapter)
{
#ifdef Q_OS_WIN
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
   

    // 初始化调用GetIfTable以获取dwSize变量中所需的大小
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        ifTable = (MIB_IFTABLE*)malloc(dwSize);
        if (ifTable == NULL)
        {
            qDebug() << "ifTable的内存分配失败";
            return false;
        }

        // 第二次调用GetIfTable以获取实际数据
        if (GetIfTable(ifTable, &dwSize, FALSE) == NO_ERROR)
        {
            for (DWORD i = 0; i < ifTable->dwNumEntries; ++i)
            {
                // 假设你要获取特定网络适配器的信息
                if (strcmp(reinterpret_cast<const char*>(ifTable->table[i].bDescr), Adapter.toStdString().c_str())== 0)
                {
                    MIB_IFROW& ifRow = ifTable->table[i];

                    // 将速度转换为每秒兆字节
     
                    if (m_recv_bytes__ == 0||m_send_bytes__==0|| ifRow.dwInOctets==0|| ifRow.dwOutOctets==0)
                    {
                        downloadSpeed = 0;
                        uploadSpeed = 0;
                    }
                    else {
                        downloadSpeed = (ifRow.dwInOctets - m_recv_bytes__) / 1000;
                        uploadSpeed = (ifRow.dwOutOctets - m_send_bytes__) / 1000;
                    }
                    qDebug() << "Download Speed:" << downloadSpeed << "kbps";
                    qDebug() << "Upload Speed:" << uploadSpeed << "kbps";

                    m_recv_bytes__ = ifRow.dwInOctets;
                    m_send_bytes__ = ifRow.dwOutOctets;

                    return true;
                }
            }
        }
        else
        {
            qDebug() << "GetIfTable调用失败";
        }

        // 释放ifTable分配的内存
    }
    else
    {
        qDebug() << "GetIfTable调用失败，需要更大的缓冲区";
    }
#endif
    return false;
}
#if defined(Q_OS_WIN32)
__int64 Filetime2Int64(const FILETIME* ftime)
{
    LARGE_INTEGER li;
    li.LowPart = ftime->dwLowDateTime;
    li.HighPart = ftime->dwHighDateTime;
    return li.QuadPart;
}

__int64 CompareFileTime(FILETIME preTime, FILETIME nowTime)
{
    return Filetime2Int64(&nowTime) - Filetime2Int64(&preTime);
}
#endif
bool MySysInfo::GetCpuUsage(double& nCpuRate)
{
    nCpuRate = -1;
#if defined(Q_OS_LINUX)
    QProcess process;
    process.start("cat /proc/stat");
    process.waitForFinished();
    QString str = process.readLine();
    str.replace("\n", "");
    str.replace(QRegExp("( ){1,}"), " ");
    auto lst = str.split(" ");
    if (lst.size() > 3)
    {
        double use = lst[1].toDouble() + lst[2].toDouble() + lst[3].toDouble();
        double total = 0;
        for (int i = 1; i < lst.size(); ++i)
            total += lst[i].toDouble();
        if (total - m_cpu_total__ > 0)
        {
            qDebug("cpu rate:%.2lf%%", (use - m_cpu_use__) / (total - m_cpu_total__) * 100.0);
            m_cpu_total__ = total;
            m_cpu_use__ = use;
            nCpuRate = (use - m_cpu_use__) / (total - m_cpu_total__) * 100.0;
            return true;
        }
    }
#else
    HANDLE hEvent;
    bool res;
    static FILETIME preIdleTime;
    static FILETIME preKernelTime;
    static FILETIME preUserTime;
    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
    preIdleTime = idleTime;
    preKernelTime = kernelTime;
    preUserTime = userTime;
    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);//初始值为nonsignaled
    WaitForSingleObject(hEvent, 1000);//等待500毫秒
    res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
    long long idle = CompareFileTime(preIdleTime, idleTime);
    long long kernel = CompareFileTime(preKernelTime, kernelTime);
    long long user = CompareFileTime(preUserTime, userTime);
    nCpuRate = ceil(100.0 * (kernel + user - idle) / (kernel + user));
    qDebug() << "windows:CPU use rate:" << nCpuRate << "%";
#endif
    return true;
}
bool MySysInfo::GetDiskSpeed()
{
#if defined(Q_OS_LINUX)
    QProcess process;
    process.start("iostat -k -d");
    if (!process.waitForStarted()) {
        qDebug() << "Failed to start iostat process";
        return false;
    }

    if (!process.waitForFinished()) {
        qDebug() << "Failed to finish iostat process";
        return false;
    }

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');

    // Assuming the relevant information is in the last line
    if (lines.size() > 1) {
        QString lastLine = lines.last();
        lastLine.replace(QRegExp("( ){1,}"), " ");
        QStringList lst = lastLine.split(" ");

        if (lst.size() > 5) {
            qDebug("disk read:%.0lfkb/s disk write:%.0lfkb/s", (lst[4].toDouble() - m_disk_read__) / (m_timer_interval__ / 1000.0), (lst[5].toDouble() - m_disk_write__) / (m_timer_interval__ / 1000.0));
            m_disk_read__ = lst[4].toDouble();
            m_disk_write__ = lst[5].toDouble();
            return true;
        }
    }
#endif
    return false;
}

bool MySysInfo::GetdiskSpace(unsigned long& lFreeAll, unsigned long& lTotalAll)
{
#if defined(Q_OS_LINUX)
    QProcess process;
    process.start("df -k");
    process.waitForFinished();
    process.readLine();
    while (!process.atEnd())
    {
        QString str = process.readLine();
        if (str.startsWith("/dev/sda"))
        {
            str.replace("\n", "");
            str.replace(QRegExp("( ){1,}"), " ");
            auto lst = str.split(" ");
            if (lst.size() > 5)
                qDebug("挂载点:%s 已用:%.0lfMB 可用:%.0lfMB", lst[5].toStdString().c_str(), lst[2].toDouble() / 1024.0, lst[3].toDouble() / 1024.0);
            lFreeAll += lst[2].toDouble() / 1024.0;
            lTotalAll += lst[3].toDouble() / 1024.0 + lFreeAll;
        }
    }
#else

    static char path[_MAX_PATH];//存储当前系统存在的盘符
    int curdrive = _getdrive();
    lFreeAll = 0UL;
    lTotalAll = 0UL;
    for (int drive = 1; drive <= curdrive; drive++)//遍历所有盘符
    {
        if (!_chdrive(drive))
        {
            sprintf(path, "%c:\\", drive + 'A' - 1);
            ULARGE_INTEGER caller, total, free;
            WCHAR wszClassName[_MAX_PATH];
            memset(wszClassName, 0, sizeof(wszClassName));
            MultiByteToWideChar(CP_ACP, 0, path, strlen(path) + 1, wszClassName,
                sizeof(wszClassName) / sizeof(wszClassName[0]));
            if (GetDiskFreeSpaceEx(wszClassName, &caller, &total, &free) == 0)
            {
                qDebug() << "GetDiskFreeSpaceEx Filed!";
                return false;
            }

            double dTepFree = free.QuadPart / (1024.0 * 1024.0);
            double dTepTotal = total.QuadPart / (1024.0 * 1024.0);
            qDebug() << "Get Windows Disk Information:" << path << "--free:" << dTepFree << "MB,--total:" << dTepTotal << "MB";
            lFreeAll += ceil(dTepFree);
            lTotalAll += ceil(dTepTotal);
        }
    }
    qDebug("Total disk capacity:%lu MB,Free disk capacity:%lu MB", lTotalAll, lFreeAll);
#endif
    return true;
}
bool MySysInfo::GetPathSpace(const QString& path)
{
#if defined(Q_OS_LINUX)
    struct statfs diskInfo;//需要#include "sys/statfs.h"
    statfs(path.toUtf8().data(), &diskInfo);
    qDebug("%s 总大小:%.0lfMB 可用大小:%.0lfMB", path.toStdString().c_str(), (diskInfo.f_blocks * diskInfo.f_bsize) / 1024.0 / 1024.0, (diskInfo.f_bavail * diskInfo.f_bsize) / 1024.0 / 1024.0);
#endif
    return true;
}
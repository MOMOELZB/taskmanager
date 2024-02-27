// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTimer>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSystemInfo();

private:
    void setupUI();
    void updateProcessInfo();
    QString getCPUUsage();
    QString getMemoryUsage();
    QString getDiskUsage();
    QString getNetworkUsage();
    QString getGPUUsage();
    QTreeWidgetItem *createProcessTreeWidgetItem(const QString &label, const QString &value);

    QVBoxLayout *centralLayout;
    QTreeWidget *treeWidget;
    QTreeWidgetItem *processItem;
};

#endif // MAINWINDOW_H

#include "TaskManager.h"
#include <QtWidgets/QApplication>
#include <Windows.h>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<QList<PROCESSENTRY32>>("QList<PROCESSENTRY32>");
    TaskManager w;
    w.show();
    return app.exec();
}

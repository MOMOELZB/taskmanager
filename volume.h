#pragma once
#pragma once
#ifndef VOLUME_H
#define VOLUME_H

#include <QObject>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
class Volume
{
public:
    Volume();


    void SetSystemVolume(int);   /*����ϵͳ����*/

    int  GetCurrentVolume();    /*��ȡϵͳ��ǰ������*/

};

#endif
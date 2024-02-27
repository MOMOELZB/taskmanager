#include "volume.h"
#include<qdebug.h>
Volume::Volume()
{

}



/*
音量调节
-2      静音
-1     取消静音
其他值为对应的音量值
*/
void Volume::SetSystemVolume(int value)
{
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator = 0;
    IMMDevice* pDevice = 0;
    IAudioEndpointVolume* pAudioEndpointVolume = 0;
    IAudioClient* pAudioClient = 0;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);

    hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) {
        // 输出错误信息
        qDebug() << "Error: " << hr;
    }
    if (value == -1)
        pAudioEndpointVolume->SetMute(FALSE, NULL);    /*取消静音*/
    else if (value == -2)
        pAudioEndpointVolume->SetMute(TRUE, NULL);   /*静音*/
    else {
        value = (value > 100) ? 100 : (value < 0) ? 0 : value;  /*将音量限制在0-100内*/
        float fVolume;
        fVolume = value / 100.0f;
        hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(fVolume, &GUID_NULL);
    }
    pAudioClient->Release();
    pAudioEndpointVolume->Release();
    pDevice->Release();
    pDeviceEnumerator->Release();
}



/*
获取当前的音量值
*/

int  Volume::GetCurrentVolume()
{
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator = 0;
    IMMDevice* pDevice = 0;
    IAudioEndpointVolume* pAudioEndpointVolume = 0;
    IAudioClient* pAudioClient = 0;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);

    hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

    float fVolume;

    hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&fVolume);


    pAudioClient->Release();
    pAudioEndpointVolume->Release();
    pDevice->Release();
    pDeviceEnumerator->Release();

    int  intVolume = fVolume * 100 + 1;
    if (intVolume > 100)
    {
        intVolume = 100;
    }
    return intVolume;

}

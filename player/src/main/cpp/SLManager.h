//
// Created by xingzhe on 2020-02-15.
//

#ifndef AUDIOPLAYER_SLMANAGER_H
#define AUDIOPLAYER_SLMANAGER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <assert.h>
#include "log_define.h"
#define TAG "SLMANAGER"

class SLManager {
public:
    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine;

    SLObjectItf outputMixObject;
    SLEnvironmentalReverbItf outputMixEnvironmentReverb;

    SLObjectItf audioPlayerObject;
    SLPlayItf audioPlayer;
    SLAndroidSimpleBufferQueueItf  playerBufferQueue;
    SLEffectSendItf playerEffectSend;
    SLVolumeItf playerVolume;

    SLManager();

    ~SLManager();

    void createEngine();

    void createBufferQueuePlayer(int sampleRate,slAndroidSimpleBufferQueueCallback callback);

    void startPlay();
};


#endif //AUDIOPLAYER_SLMANAGER_H

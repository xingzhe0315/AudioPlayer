//
// Created by xingzhe on 2020-02-15.
//

#include "SLManager.h"

void SLManager::createEngine() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"slCreateEngine error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG," (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result =  (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG," (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentReverb);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,\n"
                 "                                     &outputMixEnvironmentReverb); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    result = (*outputMixEnvironmentReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentReverb,
                                                                    &reverbSettings);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*outputMixEnvironmentReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentReverb,\n"
                 "                                                                    &reverbSettings); error!!!");
    }
}

//SLuint32 convertSampleRate(int sampleRate) {
//    SLuint32 slSampleRate = 0;
//
//    switch (sampleRate) {
//        case 8000:
//            slSampleRate = SL_SAMPLINGRATE_8;
//            break;
//        case 12000:
//            slSampleRate = SL_SAMPLINGRATE_12;
//            break;
//        case 16000:
//            slSampleRate = SL_SAMPLINGRATE_16;
//            break;
//        case 22050:
//            slSampleRate = SL_SAMPLINGRATE_22_05;
//            break;
//        case 24000:
//            slSampleRate = SL_SAMPLINGRATE_24;
//            break;
//        case 32000:
//            slSampleRate = SL_SAMPLINGRATE_32;
//            break;
//        case 44100:
//            slSampleRate = SL_SAMPLINGRATE_44_1;
//            break;
//        case 48000:
//            slSampleRate = SL_SAMPLINGRATE_48;
//            break;
//        case 64000:
//            slSampleRate = SL_SAMPLINGRATE_64;
//            break;
//        case 88200:
//            slSampleRate = SL_SAMPLINGRATE_88_2;
//            break;
//        case 96000:
//            slSampleRate = SL_SAMPLINGRATE_96;
//            break;
//        default:
//            slSampleRate = SL_SAMPLINGRATE_44_1;
//    }
//    return slSampleRate;
//}

void
SLManager::createBufferQueuePlayer(int sampleRate, slAndroidSimpleBufferQueueCallback callback) {
    SLresult result;
    SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
    };
    SLDataFormat_PCM slDataFormatPcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource dataSource = {
            &locatorBufferQueue, &slDataFormatPcm
    };
    SLDataLocator_OutputMix locatorOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink dataSink = {&locatorOutputMix, nullptr};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ };
    result = (*engineEngine)->CreateAudioPlayer(
            engineEngine,
            &audioPlayerObject,
            &dataSource,
            &dataSink,
            3,
            ids,
            req
    );
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*engineEngine)->CreateAudioPlayer error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_PLAY, &audioPlayer);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_PLAY, &audioPlayer); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, callback, nullptr);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*playerBufferQueue)->RegisterCallback(playerBufferQueue, callback, nullptr); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_EFFECTSEND, &playerEffectSend);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_EFFECTSEND, &playerEffectSend); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_VOLUME, &playerVolume);
    if(SL_RESULT_SUCCESS != result){
        LOGE(TAG,"(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_VOLUME, &playerVolume); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
}

SLManager::SLManager() {
    createEngine();
}

SLManager::~SLManager() {

}

void SLManager::startPlay() {
    (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PLAYING);

}

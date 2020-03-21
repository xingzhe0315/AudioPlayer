//
// Created by xingzhe on 2020-02-13.
//
#ifndef AUDIOPLAYER_AUDIOPLAYER_H
#define AUDIOPLAYER_AUDIOPLAYER_H

#define TAG "AUDIO_PLAYER"

#include <jni.h>
#include "log_define.h"
#include <pthread.h>
#include "AudioPacketQueue.h"

#include <iostream>
#include <cstring>
#include <assert.h>
#include <SoundTouch.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

#define PLAY_STATUS_PREPARED 1
#define PLAY_STATUS_LOADING 2
#define PLAY_STATUS_PLAYING 3
#define PLAY_STATUS_PAUSED 4
#define PLAY_STATUS_STOPED 5
#define PLAY_STATUS_FINISHED 6

#define THREAD_MAIN 1 // 主线程
#define THREAD_WORK 2 // 子线程

using namespace soundtouch;

class AudioPlayerListener {
public:
    virtual void notify(int what, int thread) = 0;

    virtual void updateTime(int duration, double progress, int thread) = 0;
};

class AudioPlayer {
public:
    const char *mDataSource;
    pthread_t prepareThread;
    pthread_t decodeThread;
    pthread_t playThread;
    AudioPlayerListener *mListener;
    AVFormatContext *avFormatContext;
    AVCodecContext *avCodecContext;
    SwrContext *swrContext;
    int audioIndex;
    AudioPacketQueue *mPacketQueue;
    uint8_t *buffer;
    int status;
    volatile bool decodeFinished;

    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine;

    SLObjectItf outputMixObject;
    SLEnvironmentalReverbItf outputMixEnvironmentReverb;

    SLObjectItf audioPlayerObject;
    SLPlayItf audioPlayer;
    SLAndroidSimpleBufferQueueItf playerBufferQueue;

    SLEffectSendItf playerEffectSend;
    SLVolumeItf playerVolume;
    SLMuteSoloItf playerMuteSolo;
    volatile bool mPlayerInitialized = false;

    int duration;
    AVRational time_base;
    double clock;
    double lastClock;

    volatile bool exit = false;
    pthread_mutex_t seekMutex;
    pthread_mutex_t prepareMutex;
    pthread_mutex_t decodeMutex;

    SoundTouch *soundTouch = nullptr;
    SAMPLETYPE *sampleBuffer = nullptr;
    int sampleNumPerChannel = 0;
    float pitch = 1.0f;
    float speed = 1.0f;
    int mVolume = 100;
    int mSoundChannel = 3;
    bool mPacketReadFinished = true;
    AVPacket *mAvPacket;
    int mSeekTime;
    pthread_t mSeekThread;

    AudioPlayer();

    ~AudioPlayer();

    void setListener(AudioPlayerListener *listener);

    void setDataSource(const char *dataSource);

    void prepare();

    void prepareAsync();

    void start();

    void stop();

    void release();

    void pause();

    int resampleFrame();

    int getSoundTouchData();

    void createEngine();

    void createBufferQueuePlayer(int sampleRate);

    void startPlay();

    void resume();

    void seekTo(jint time);

    void setVolume(jint volume);

    void setSoundChannel(jint channel);

    void initSoundTouch(int sampleRate);

    void setPitch(float pitch);

    void setSpeed(float speed);
};


#endif //AUDIOPLAYER_AUDIOPLAYER_H

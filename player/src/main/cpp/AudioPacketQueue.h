//
// Created by xingzhe on 2020-02-13.
//

#ifndef AUDIOPLAYER_AUDIOPACKETQUEUE_H
#define AUDIOPLAYER_AUDIOPACKETQUEUE_H

#include "queue"
#include <pthread.h>
#include "log_define.h"

#define TAG "AudioPacketQueue"
#define PACKET_QUEUE_CAPACITY 200
#define PACKET_QUEUE_LOW_LIMIT 50

extern "C" {
#include <libavcodec/avcodec.h>
};

class AudioPacketQueue {
public:
    std::queue<AVPacket *> mPacketQueue;
    pthread_mutex_t mPacketMutex;
    pthread_cond_t mEmptyCond;
    pthread_cond_t mFullCond;

    AudioPacketQueue();

    ~AudioPacketQueue();

    int putAudioPacket(AVPacket *packet);

    int getAudioPacket(AVPacket *packet);

    int getQueueSize();

    void clear(double ts);
};


#endif //AUDIOPLAYER_AUDIOPACKETQUEUE_H

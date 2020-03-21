//
// Created by xingzhe on 2020-02-13.
//

#include "AudioPacketQueue.h"

AudioPacketQueue::AudioPacketQueue() {
    pthread_mutex_init(&mPacketMutex, nullptr);
    pthread_cond_init(&mEmptyCond, nullptr);
    pthread_cond_init(&mFullCond, nullptr);
}

AudioPacketQueue::~AudioPacketQueue() {
    pthread_mutex_destroy(&mPacketMutex);
    pthread_cond_destroy(&mEmptyCond);
    pthread_cond_destroy(&mFullCond);
}

int AudioPacketQueue::putAudioPacket(AVPacket *packet) {
    pthread_mutex_lock(&mPacketMutex);
    mPacketQueue.push(packet);
    if (mPacketQueue.size() > PACKET_QUEUE_CAPACITY) {
        LOGE(TAG, "the buffer queue is full , decode thread wait");
        pthread_cond_wait(&mFullCond, &mPacketMutex);
    }
    pthread_cond_signal(&mEmptyCond);
    pthread_mutex_unlock(&mPacketMutex);
    return 0;
}

int AudioPacketQueue::getAudioPacket(AVPacket *packet) {
    pthread_mutex_lock(&mPacketMutex);
    if (!mPacketQueue.empty()) {
        AVPacket *src = mPacketQueue.front();
        if (av_packet_ref(packet, src) == 0) {
            mPacketQueue.pop();
        }
        av_packet_free(&src);
        av_free(src);
        src = nullptr;
        if (mPacketQueue.size() < PACKET_QUEUE_LOW_LIMIT) {
            LOGE(TAG, "the buffer queue is not enough , decode thread weakup");
            pthread_cond_signal(&mFullCond);
        }
    } else {
        pthread_cond_wait(&mEmptyCond, &mPacketMutex);
    }
    pthread_mutex_unlock(&mPacketMutex);
    return 0;
}

int AudioPacketQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mPacketMutex);
    size = mPacketQueue.size();
    pthread_mutex_unlock(&mPacketMutex);
    return size;
}

void AudioPacketQueue::clear(double ts) {

    pthread_mutex_lock(&mPacketMutex);
    while (!mPacketQueue.empty()) {
        AVPacket *packet = mPacketQueue.front();
        if (packet->pts >= ts) {
            LOGE("AUDIO_PLAYER", "current packet pts = %d", packet->pts)
            break;
        }
        mPacketQueue.pop();
        av_packet_free(&packet);
        packet = nullptr;
    }
    pthread_cond_signal(&mFullCond);
    pthread_mutex_unlock(&mPacketMutex);
}

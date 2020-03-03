//
// Created by xingzhe on 2020-02-13.
//

#include "AudioPlayer.h"


SLuint32 convertSampleRate(int sampleRate) {
    SLuint32 slSampleRate = 0;

    switch (sampleRate) {
        case 8000:
            slSampleRate = SL_SAMPLINGRATE_8;
            break;
        case 12000:
            slSampleRate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            slSampleRate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            slSampleRate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            slSampleRate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            slSampleRate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            slSampleRate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            slSampleRate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            slSampleRate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            slSampleRate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            slSampleRate = SL_SAMPLINGRATE_96;
            break;
        default:
            slSampleRate = SL_SAMPLINGRATE_44_1;
    }
    return slSampleRate;
}


void playCallback(SLAndroidSimpleBufferQueueItf caller,
                  void *pContext) {
    auto *audioPlayer = static_cast<AudioPlayer *>(pContext);
    int dataSize = audioPlayer->resampleFrame(&audioPlayer->buffer);
    if (dataSize != 0) {
        audioPlayer->clock +=
                ((double) dataSize) / audioPlayer->avCodecContext->sample_rate * 2 * 2;
        audioPlayer->mListener->updateTime(audioPlayer->duration, audioPlayer->clock, THREAD_WORK);
        (*audioPlayer->playerBufferQueue)->Enqueue(audioPlayer->playerBufferQueue,
                                                   audioPlayer->buffer,
                                                   static_cast<SLuint32>(dataSize));
    }
}

void AudioPlayer::setDataSource(const char *dataSource) {
    this->mDataSource = dataSource;
}

int AudioPlayer::resampleFrame(uint8_t **inBuffer) {
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    int dataSize = 0;
    while (status == PLAY_STATUS_PLAYING || status == PLAY_STATUS_LOADING) {
        if (mPacketQueue->getQueueSize() <= 0) {
            if (decodeFinished) {
                mListener->notify(PLAY_STATUS_FINISHED, THREAD_WORK);
                break;
            }
            if (status != PLAY_STATUS_LOADING) {
                status = PLAY_STATUS_LOADING;
                mListener->notify(PLAY_STATUS_LOADING, THREAD_WORK);
            }
            continue;
        } else {
            if (status != PLAY_STATUS_PLAYING) {
                status = PLAY_STATUS_PLAYING;
                mListener->notify(PLAY_STATUS_PLAYING, THREAD_WORK);
            }
        }
        if (exit) {
            break;
        }
        mPacketQueue->getAudioPacket(avPacket);
        if (avcodec_send_packet(avCodecContext, avPacket) == 0
            && avcodec_receive_frame(avCodecContext, avFrame) == 0) {
            int nb = swr_convert(swrContext,
                                 inBuffer,
                                 avFrame->nb_samples,
                                 reinterpret_cast<const uint8_t **>(&avFrame->data),
                                 avFrame->nb_samples);
            int channelNumber = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            dataSize = av_samples_get_buffer_size(
                    nullptr,
                    channelNumber,
                    nb,
                    AV_SAMPLE_FMT_S16,
                    1);
            clock = avFrame->pts * av_q2d(time_base);
            break;
        }
    }

    return dataSize;
}


bool _prepare(AudioPlayer *audioPlayer) {
    const char *dataSource = audioPlayer->mDataSource;
    if (dataSource == nullptr) {
        LOGE(TAG, "mDataSource can not be null");
        return false;
    }
    AVFormatContext *avFormatContext = avformat_alloc_context();
    int result = -1;
    result = avformat_open_input(&avFormatContext, dataSource, nullptr, nullptr);
    if (result != 0) {
        LOGE(TAG, "avformat_open_input error : %d", result);
        return false;
    }
    result = avformat_find_stream_info(avFormatContext, nullptr);
    if (result < 0) {
        LOGE(TAG, "avformat_find_stream_info error : %d", result);
        return false;
    }
    int audioIndex = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *avStream = avFormatContext->streams[i];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioPlayer->duration = avFormatContext->duration / AV_TIME_BASE;
            audioPlayer->time_base = avStream->time_base;
            LOGE(TAG, "tootle tome = %d", audioPlayer->duration);
            LOGE(TAG, "stream time base num = %d ; den = %d ", avStream->time_base.num,
                 avStream->time_base.den);
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1) {
        LOGE(TAG, "failed to find audio stream!!!");
        return false;
    }
    AVCodecContext *avCodecContext = avcodec_alloc_context3(nullptr);
    result = avcodec_parameters_to_context(avCodecContext,
                                           avFormatContext->streams[audioIndex]->codecpar);
    if (result < 0) {
        LOGE(TAG, "avcodec_parameters_to_context error : %d", result);
        return false;
    }
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    if (avCodec == nullptr) {
        LOGE(TAG, "avcodec_find_decoder failed!!!");
        return false;
    }

    result = avcodec_open2(avCodecContext, avCodec, nullptr);
    if (result != 0) {
        LOGE(TAG, "avcodec_open2 error:%d", result);
        return false;
    }
    int sampleRate = avCodecContext->sample_rate;
    AVSampleFormat inSampleFormat = avCodecContext->sample_fmt;
    uint64_t inChannelLayout = avCodecContext->channel_layout;
    SwrContext *swrContext = swr_alloc_set_opts(
            nullptr,
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_S16,
            sampleRate,
            inChannelLayout,
            inSampleFormat,
            sampleRate,
            0, nullptr);
    if (!swrContext || swr_init(swrContext)) {
        LOGE(TAG, "swrcontext init error");
        return false;
    }
    audioPlayer->createBufferQueuePlayer(avCodecContext->sample_rate);
    audioPlayer->avFormatContext = avFormatContext;
    audioPlayer->avCodecContext = avCodecContext;
    audioPlayer->audioIndex = audioIndex;
    audioPlayer->status = PLAY_STATUS_PREPARED;
    audioPlayer->swrContext = swrContext;
    LOGE(TAG, "audio player prepared!");
    return true;
}

void *_prepareAsync(void *data) {
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    if (_prepare(audioPlayer)) {
        audioPlayer->mListener->notify(1, THREAD_WORK);
    }
    pthread_exit(&audioPlayer->prepareThread);
}

void AudioPlayer::prepare() {
    _prepare(this);
}

void AudioPlayer::prepareAsync() {
    pthread_create(&prepareThread, nullptr, _prepareAsync, this);
}

void *decode(void *data) {
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    AVPacket *avPacket = av_packet_alloc();
    while (av_read_frame(audioPlayer->avFormatContext, avPacket) == 0 && !audioPlayer->exit) {
        if (avPacket->stream_index == audioPlayer->audioIndex) {
            audioPlayer->mPacketQueue->putAudioPacket(avPacket);
        }
        avPacket = av_packet_alloc();
    }
    LOGE(TAG, "解码完成！");
    audioPlayer->decodeFinished = true;
    pthread_exit(&audioPlayer->decodeThread);
}

void *play(void *data) {
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    audioPlayer->startPlay();
    playCallback(audioPlayer->playerBufferQueue, audioPlayer);
    pthread_exit(&audioPlayer->playThread);
}

void AudioPlayer::start() {
    if (status != PLAY_STATUS_PREPARED) {
        LOGE(TAG, "AudioPlayer not prepared!!!");
        return;
    }
    if (status == PLAY_STATUS_PLAYING) {
        LOGE(TAG, "AudioPlayer is already playing!!!");
        return;
    }
    exit = false;
    mListener->notify(PLAY_STATUS_PLAYING, THREAD_MAIN);
    status = PLAY_STATUS_PLAYING;
    pthread_create(&decodeThread, nullptr, decode, this);
    pthread_create(&playThread, nullptr, play, this);
}

void AudioPlayer::stop() {
    if (audioPlayer != nullptr) {
        (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_STOPPED);
    }
    status = PLAY_STATUS_STOPED;
}


void AudioPlayer::release() {
    stop();
    exit = true;
    if (mPacketQueue != nullptr) {
        mPacketQueue->clear(INT64_MAX);
    }
    if (audioPlayerObject != nullptr) {
        (*audioPlayerObject)->Destroy(audioPlayerObject);
        audioPlayerObject = nullptr;
        audioPlayer = nullptr;
        playerBufferQueue = nullptr;

    }

    if (avCodecContext != nullptr) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = nullptr;
    }
    if (avFormatContext != nullptr) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = nullptr;
    }

}


void AudioPlayer::pause() {
    if (status == PLAY_STATUS_PLAYING) {
        (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PAUSED);
        status = PLAY_STATUS_PAUSED;
    }
}

void AudioPlayer::resume() {
    if (status == PLAY_STATUS_PAUSED) {
        (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PLAYING);
        status = PLAY_STATUS_PLAYING;
    }
}


AudioPlayer::AudioPlayer() {
    mPacketQueue = new AudioPacketQueue();
    buffer = static_cast<uint8_t *>(av_malloc(44100 * 2 * 2));
    createEngine();
}

AudioPlayer::~AudioPlayer() {
    delete (mListener);
    mListener = nullptr;

    if (buffer != nullptr) {
        delete (buffer);
        buffer = nullptr;
    }
    if (outputMixObject != nullptr) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
        outputMixEnvironmentReverb = nullptr;
    }

    if (engineObject != nullptr) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineEngine = nullptr;
    }
    if (mPacketQueue != nullptr) {
        mPacketQueue->clear(INT64_MAX);
        mPacketQueue = nullptr;
    }
}

void AudioPlayer::setListener(AudioPlayerListener *listener) {
    this->mListener = listener;
}

void AudioPlayer::createEngine() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "slCreateEngine error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "(*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE) error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             " (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, " (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentReverb);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "(*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,\n"
                  "                                     &outputMixEnvironmentReverb); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    result = (*outputMixEnvironmentReverb)->SetEnvironmentalReverbProperties(
            outputMixEnvironmentReverb,
            &reverbSettings);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*outputMixEnvironmentReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentReverb,\n"
             "                                                                    &reverbSettings); error!!!");
    }
}


void AudioPlayer::createBufferQueuePlayer(int sampleRate) {
    SLresult result;
    SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
    };
    SLDataFormat_PCM slDataFormatPcm = {
            SL_DATAFORMAT_PCM,
            2,
            convertSampleRate(sampleRate),
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
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "(*engineEngine)->CreateAudioPlayer error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "(*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_PLAY, &audioPlayer);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_PLAY, &audioPlayer); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_BUFFERQUEUE,
                                                &playerBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playCallback, this);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*playerBufferQueue)->RegisterCallback(playerBufferQueue, callback, nullptr); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_EFFECTSEND,
                                                &playerEffectSend);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_EFFECTSEND, &playerEffectSend); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_VOLUME, &playerVolume);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_VOLUME, &playerVolume); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
}

void AudioPlayer::startPlay() {
    (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PLAYING);
}

void AudioPlayer::seekTo(int time) {
    if (time < 0 || time > duration) {
        return;
    }
    auto ts = ((double) time) / av_q2d(time_base);
    LOGE(TAG, "seek to : time =  %f", ts);
    if (time > clock) {
        mPacketQueue->clear(ts);
    } else {
        mPacketQueue->clear(INT64_MAX);
        avformat_seek_file(avFormatContext, audioIndex, INT64_MIN, ts, INT64_MAX, 0);
        if (decodeFinished) {
            decodeFinished = false;
            pthread_create(&decodeThread, nullptr, decode, this);
        }
    }
//    pthread_mutex_lock(&seekMutex);

//    pthread_mutex_unlock(&seekMutex);


//    mPacketQueue->clear(ts);
//    clock = 0;
    av_seek_frame(avFormatContext, audioIndex, static_cast<int64_t>(ts), AVSEEK_FLAG_BACKWARD);
}






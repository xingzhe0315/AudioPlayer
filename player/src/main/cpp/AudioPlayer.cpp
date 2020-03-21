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
    int dataSize = audioPlayer->getSoundTouchData();
    if (dataSize != 0) {
        audioPlayer->clock +=
                ((double) dataSize) / audioPlayer->avCodecContext->sample_rate * 2 * 2;
        audioPlayer->mListener->updateTime(audioPlayer->duration, audioPlayer->clock, THREAD_WORK);
        (*audioPlayer->playerBufferQueue)->Enqueue(audioPlayer->playerBufferQueue,
                                                   audioPlayer->sampleBuffer,
                                                   static_cast<SLuint32>(dataSize * 4));
    }
}

void AudioPlayer::setDataSource(const char *dataSource) {
    this->mDataSource = dataSource;
}

int AudioPlayer::resampleFrame() {
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    int dataSize = 0;
    while (status == PLAY_STATUS_PLAYING || status == PLAY_STATUS_LOADING) {
        if (mPacketQueue->getQueueSize() <= 0) {
            if (decodeFinished) {
                status = PLAY_STATUS_FINISHED;
                release();
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
            sampleNumPerChannel = swr_convert(swrContext,
                                              &buffer,
                                              avFrame->nb_samples,
                                              reinterpret_cast<const uint8_t **>(&avFrame->data),
                                              avFrame->nb_samples);
            int channelNum = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            dataSize = av_samples_get_buffer_size(
                    nullptr,
                    channelNum,
                    sampleNumPerChannel,
                    AV_SAMPLE_FMT_S16,
                    1);
            clock = avFrame->pts * av_q2d(time_base);
            break;
        }
    }
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);
    avPacket = nullptr;
    avFrame = nullptr;
    return dataSize;
}

int AudioPlayer::getSoundTouchData() {
    while (status == PLAY_STATUS_PLAYING || status == PLAY_STATUS_LOADING) {
        int dataSize = resampleFrame();
        int sampleNum = sampleNumPerChannel;
        for (int i = 0; i < dataSize / 2 + 1; ++i) {
            sampleBuffer[i] = (buffer[2 * i] | ((buffer[2 * i + 1]) << 8));
        }
        soundTouch->putSamples(sampleBuffer, sampleNumPerChannel);
        sampleNum = soundTouch->receiveSamples(sampleBuffer, dataSize / 4);
        if (sampleNum == 0) {
            continue;
        } else {
            return sampleNum;
        }
    }
    return 0;
}


bool _prepare(AudioPlayer *audioPlayer) {
    pthread_mutex_lock(&audioPlayer->prepareMutex);
    LOGE(TAG, "start prepare");
    const char *dataSource = audioPlayer->mDataSource;
    if (dataSource == nullptr) {
        LOGE(TAG, "mDataSource can not be null");
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    LOGE(TAG, "get data source success %s", dataSource);
    AVFormatContext *avFormatContext = avformat_alloc_context();
    LOGE(TAG, "avformat inited!!!");
    int result = -1;
    result = avformat_open_input(&avFormatContext, dataSource, nullptr, nullptr);
    if (result != 0) {
        LOGE(TAG, "avformat_open_input error : %d", result);
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    LOGE(TAG, "open input success!!!");
    result = avformat_find_stream_info(avFormatContext, nullptr);
    if (result < 0) {
        LOGE(TAG, "avformat_find_stream_info error : %d", result);
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    LOGE(TAG, "find stream info success");
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
    LOGE(TAG, "get the audio track indes");
    if (audioIndex == -1) {
        LOGE(TAG, "failed to find audio stream!!!");
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    AVCodecContext *avCodecContext = avcodec_alloc_context3(nullptr);
    result = avcodec_parameters_to_context(avCodecContext,
                                           avFormatContext->streams[audioIndex]->codecpar);
    if (result < 0) {
        LOGE(TAG, "avcodec_parameters_to_context error : %d", result);
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    if (avCodec == nullptr) {
        LOGE(TAG, "avcodec_find_decoder failed!!!");
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }

    result = avcodec_open2(avCodecContext, avCodec, nullptr);
    if (result != 0) {
        LOGE(TAG, "avcodec_open2 error:%d", result);
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
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
        pthread_mutex_unlock(&audioPlayer->prepareMutex);
        return false;
    }
    audioPlayer->initSoundTouch(avCodecContext->sample_rate);
    audioPlayer->createBufferQueuePlayer(avCodecContext->sample_rate);
    audioPlayer->avFormatContext = avFormatContext;
    audioPlayer->avCodecContext = avCodecContext;
    audioPlayer->audioIndex = audioIndex;
    audioPlayer->status = PLAY_STATUS_PREPARED;
    audioPlayer->swrContext = swrContext;
    LOGE(TAG, "audio player prepared!");
    pthread_mutex_unlock(&audioPlayer->prepareMutex);
    return true;
}

void *_prepareAsync(void *data) {
    LOGE(TAG, "############ begin _prepareAsync ############");
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    if (_prepare(audioPlayer)) {
        audioPlayer->mListener->notify(1, THREAD_WORK);
    }
    LOGE(TAG, "############ end _prepareAsync ############");
    pthread_exit(&audioPlayer->prepareThread);
}

void AudioPlayer::prepare() {
    _prepare(this);
}

void AudioPlayer::prepareAsync() {
    pthread_create(&prepareThread, nullptr, _prepareAsync, this);
}

void *decode(void *data) {
    LOGE(TAG, "############ begin decode ############");
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    audioPlayer->decodeFinished = false;
    AVPacket *avPacket = av_packet_alloc();
    while (!audioPlayer->exit && audioPlayer->avFormatContext != nullptr) {
        pthread_mutex_lock(&audioPlayer->decodeMutex);
        pthread_mutex_lock(&audioPlayer->seekMutex);
        int ret = 0;
        if (audioPlayer->avFormatContext != nullptr) {
            ret = av_read_frame(audioPlayer->avFormatContext, avPacket);
        }
        pthread_mutex_unlock(&audioPlayer->seekMutex);
        pthread_mutex_unlock(&audioPlayer->decodeMutex);
        if (ret == 0 && !audioPlayer->exit) {
            if (avPacket->stream_index == audioPlayer->audioIndex) {
                LOGE(TAG, "decode thread working");
                audioPlayer->mPacketQueue->putAudioPacket(avPacket);
            }
            avPacket = av_packet_alloc();
        } else {
            break;
        }
    }
    LOGE(TAG, "解码完成！");
    audioPlayer->decodeFinished = true;
    pthread_exit(&audioPlayer->decodeThread);
}

void *play(void *data) {
    LOGE(TAG, "############ begin play ############");
    auto *audioPlayer = static_cast<AudioPlayer *>(data);
    audioPlayer->startPlay();
    playCallback(audioPlayer->playerBufferQueue, audioPlayer);
    LOGE(TAG, "############ end play ############");
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
    LOGE(TAG, "############ begin start ############");
    exit = false;
    mListener->notify(PLAY_STATUS_PLAYING, THREAD_MAIN);
    status = PLAY_STATUS_PLAYING;
    pthread_create(&decodeThread, nullptr, decode, this);
    pthread_create(&playThread, nullptr, play, this);
}

void AudioPlayer::stop() {
    LOGE(TAG, "############ begin stop ############");
    if (audioPlayer != nullptr) {
        (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_STOPPED);
    }
    status = PLAY_STATUS_STOPED;
    LOGE(TAG, "############ end stop ############");
}


void AudioPlayer::release() {
    LOGE(TAG, "############ begin release ############");
    exit = true;
    LOGE(TAG, "############ set exit to true ############");
    pthread_mutex_lock(&prepareMutex);
    pthread_mutex_lock(&decodeMutex);
    stop();
    if (mPacketQueue != nullptr) {
        mPacketQueue->clear(INT64_MAX);
        LOGE(TAG, "mPacketQueue cleared!!!");
    }
    if (audioPlayerObject != nullptr) {
        (*audioPlayerObject)->Destroy(audioPlayerObject);
        audioPlayerObject = nullptr;
        audioPlayer = nullptr;
        playerBufferQueue = nullptr;
        LOGE(TAG, "audioPlayerObject released!!!");
    }

    if (avCodecContext != nullptr) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = nullptr;
        LOGE(TAG, "avCodecContext freed!!!");
    }
    if (avFormatContext != nullptr) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = nullptr;
        LOGE(TAG, "avFormatContext freed!!!");
    }
    if (swrContext != nullptr) {
        swr_close(swrContext);
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    if (soundTouch != nullptr) {
        delete (soundTouch);
        soundTouch = nullptr;
    }
    if (sampleBuffer != nullptr) {
        free(sampleBuffer);
        sampleBuffer = nullptr;
    }
    LOGE(TAG, "############ end release ############");
    pthread_mutex_unlock(&decodeMutex);
    pthread_mutex_unlock(&prepareMutex);
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
    pthread_mutex_init(&seekMutex, nullptr);
    pthread_mutex_init(&decodeMutex, nullptr);
    pthread_mutex_init(&prepareMutex, nullptr);
}

AudioPlayer::~AudioPlayer() {
    delete (mListener);
    mListener = nullptr;

    if (buffer != nullptr) {
        free(buffer);
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
    pthread_mutex_destroy(&seekMutex);
    pthread_mutex_destroy(&decodeMutex);
    pthread_mutex_destroy(&prepareMutex);
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

    const SLInterfaceID ids[5] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
                                  SL_IID_PLAYBACKRATE,
                                  SL_IID_MUTESOLO};
    const SLboolean req[5] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                              SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(
            engineEngine,
            &audioPlayerObject,
            &dataSource,
            &dataSink,
            5,
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
    setVolume(mVolume);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_VOLUME, &playerVolume); error!!!");
    }
    assert(SL_RESULT_SUCCESS == result);
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_MUTESOLO,
                                                &playerMuteSolo);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,
             "(*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_MUTESOLO, &playerMuteSolo); error!!!");
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
    pthread_mutex_lock(&seekMutex);
    auto ts = ((double) time) / av_q2d(time_base);
    LOGE(TAG, "seek to : time =  %f", ts);
    if (time > clock) {
        mPacketQueue->clear(ts);
    } else {
        if (decodeFinished) {
            decodeFinished = false;
            pthread_create(&decodeThread, nullptr, decode, this);
        }
        mPacketQueue->clear(INT64_MAX);
    }
    avformat_seek_file(avFormatContext, audioIndex, INT64_MIN, ts, INT64_MAX, 0);
    pthread_mutex_unlock(&seekMutex);
}

void AudioPlayer::setVolume(int percent) {
    mVolume = percent;
    if (playerVolume != NULL) {
        if (percent > 30) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -20);
        } else if (percent > 25) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -22);
        } else if (percent > 20) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -25);
        } else if (percent > 15) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -28);
        } else if (percent > 10) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -30);
        } else if (percent > 5) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -34);
        } else if (percent > 3) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -37);
        } else if (percent > 0) {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -40);
        } else {
            (*playerVolume)->SetVolumeLevel(playerVolume, (100 - percent) * -100);
        }
    }
}

void AudioPlayer::setSoundChannel(jint channel) {
    if (channel == 0)//center
    {
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 1, false);
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 0, false);
    } else if (channel == 1)//left
    {
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 1, true);
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 0, false);
    } else if (channel == 2)//right
    {
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 1, false);
        (*playerMuteSolo)->SetChannelMute(playerMuteSolo, 0, true);
    }
}

void AudioPlayer::initSoundTouch(int sampleRate) {
    soundTouch = new SoundTouch();
    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sampleRate * 4));
    soundTouch->setSampleRate(sampleRate);
    soundTouch->setChannels(2);
    soundTouch->setPitch(pitch);
    soundTouch->setTempo(speed);
}

void AudioPlayer::setPitch(float pitch) {
    this->pitch = pitch;
    if (soundTouch != nullptr) {
        soundTouch->setPitch(pitch);
    }
}

void AudioPlayer::setSpeed(float speed) {
    this->speed = speed;
    if (soundTouch != nullptr) {
        soundTouch->setTempo(speed);
    }
}








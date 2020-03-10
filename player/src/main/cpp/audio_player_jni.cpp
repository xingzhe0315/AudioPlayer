#include <jni.h>
#include <string>
#include "log_define.h"
#include "AudioPlayer.h"

#define TAG "AUDIO_PLAYER_JNI"

JavaVM *javaVm;


struct AudioPlayerJava {
    jmethodID postEvent;
    jmethodID updateTime;
} audioPlayerJava;

void callJavaNotify(JNIEnv *env, jobject target, int what) {
    env->CallVoidMethod(target, audioPlayerJava.postEvent, what);
}

class JNIAudioPlayerListener : public AudioPlayerListener {
public:
    JNIAudioPlayerListener(JNIEnv *env, jobject obj);

    ~JNIAudioPlayerListener();

    void notify(int what, int thread) override;

    void updateTime(int duration, double progress, int thread);

private:
    jobject mTarget;
};


JNIAudioPlayerListener::JNIAudioPlayerListener(JNIEnv *env, jobject obj) {
    mTarget = env->NewGlobalRef(obj);
}

JNIAudioPlayerListener::~JNIAudioPlayerListener() {
    JNIEnv *env;
    javaVm->AttachCurrentThread(&env, nullptr);
    env->DeleteGlobalRef(mTarget);
}

void JNIAudioPlayerListener::notify(int what, int thread) {
    JNIEnv *env;
    switch (thread) {
        case THREAD_WORK:
            javaVm->AttachCurrentThread(&env, nullptr);
            callJavaNotify(env, mTarget, what);
            javaVm->DetachCurrentThread();
            break;
        case THREAD_MAIN:
            javaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            callJavaNotify(env, mTarget, what);
            break;
    }
}

void JNIAudioPlayerListener::updateTime(int duration, double progress, int thread) {
    JNIEnv *env;
    switch (thread) {
        case THREAD_WORK:
            javaVm->AttachCurrentThread(&env, nullptr);
            env->CallVoidMethod(mTarget, audioPlayerJava.updateTime, duration, progress);
            javaVm->DetachCurrentThread();
            break;
        case THREAD_MAIN:
            javaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            env->CallVoidMethod(mTarget, audioPlayerJava.updateTime, duration, progress);
            break;
    }
}

AudioPlayer *getAudioPlayer(jlong nativePtr) {
    return reinterpret_cast<AudioPlayer *>(nativePtr);
}

extern "C" JNIEXPORT void JNICALL
setDataSource(JNIEnv *env, jobject thiz, jstring dataSource, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->setDataSource(env->GetStringUTFChars(dataSource, 0));
}
extern "C"
JNIEXPORT void JNICALL
prepare(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->prepare();
}

extern "C" JNIEXPORT void JNICALL
prepareAsync(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->prepareAsync();
}
extern "C"
JNIEXPORT void JNICALL
start(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->start();
}
extern "C"
JNIEXPORT void JNICALL
stop(JNIEnv *env, jobject thiz, jlong nativePtr) {
    LOGE(TAG, "############ begin jni stop ############");
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->release();
}
extern "C"
JNIEXPORT void JNICALL
pause(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->pause();
}

extern "C"
JNIEXPORT void JNICALL
resume(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->resume();
}
extern "C"
JNIEXPORT void JNICALL
release(JNIEnv *env, jobject thiz, jlong nativePtr) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    delete (audioPlayer);
}

extern "C"
JNIEXPORT void JNICALL
seekTo(JNIEnv *env, jobject thiz, jlong nativePtr, jint time) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->seekTo(time);
}

extern "C"
JNIEXPORT void JNICALL
setVolume(JNIEnv *env, jobject obj, jlong nativePtr, jint volume) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->setVolume(volume);
}

extern "C"
JNIEXPORT void JNICALL
setSoundChannel(JNIEnv *env, jobject obj, jlong nativePtr, jint channel) {
    AudioPlayer *audioPlayer = getAudioPlayer(nativePtr);
    audioPlayer->setSoundChannel(channel);
}

extern "C"
JNIEXPORT void JNICALL
native_init(JNIEnv *env, jobject thiz) {
    auto *audioPlayer = new AudioPlayer();
    auto *listener = new JNIAudioPlayerListener(env, thiz);
    audioPlayer->setListener(listener);
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz != nullptr) {
        jfieldID nativePtrFieldId = env->GetFieldID(clazz, "mNativePtr", "J");
        if (nativePtrFieldId == nullptr) {
            return;
        }
        env->SetLongField(thiz, nativePtrFieldId, reinterpret_cast<jlong>(audioPlayer));
    }
}

const JNINativeMethod nativeMethod[] = {
        {"_native_init",     "()V",                    (void *) native_init},
        {"_setDataSource",   "(Ljava/lang/String;J)V", (void *) setDataSource},
        {"_prepare",         "(J)V",                   (void *) prepare},
        {"_prepareAsync",    "(J)V",                   (void *) prepareAsync},
        {"_start",           "(J)V",                   (void *) start},
        {"_pause",           "(J)V",                   (void *) pause},
        {"_resume",          "(J)V",                   (void *) resume},
        {"_stop",            "(J)V",                   (void *) stop},
        {"_release",         "(J)V",                   (void *) release},
        {"_seekTo",          "(JI)V",                  (void *) seekTo},
        {"_setVolume",       "(JI)V",                  (void *) setVolume},
        {"_setSoundChannel", "(JI)V",                  (void *) setSoundChannel}
};

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reversed) {
    const char *className = "com/example/player/AudioPlayer";
    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    jclass clazz = env->FindClass(className);

    if (clazz == nullptr) {
        return -1;
    }
    audioPlayerJava.postEvent = env->GetMethodID(clazz, "postEventFromNative", "(I)V");
    audioPlayerJava.updateTime = env->GetMethodID(clazz, "onTimeUpdate", "(ID)V");
    if (env->RegisterNatives(clazz, nativeMethod, sizeof(nativeMethod) / sizeof(nativeMethod[0])) <
        0) {
        return -1;
    }
    return JNI_VERSION_1_6;

}
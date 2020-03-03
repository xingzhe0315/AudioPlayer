package com.example.audioplayer;

import android.os.Handler;
import android.os.Message;

import androidx.annotation.NonNull;

import com.example.audioplayer.bean.TimeMetaData;

import java.lang.ref.WeakReference;

public class AudioPlayer {
    static {
        System.loadLibrary("audio-player");
    }

    private final static int STATE_PREPARED = 1;
    private final static int STATE_LOADING = 2;
    private final static int STATE_PLAYING = 3;
    private final static int STATE_PAUSING = 4;
    private final static int STATE_STOPED = 5;
    private final static int STATE_FINISHED = 6;

    private long mNativePtr;
    private OnStateChangeListener mOnStateChangeListener;
    private EventHandler mEventHandler;
    private TimeMetaData mTimeMetaData;

    public AudioPlayer() {
        mEventHandler = new EventHandler(this);
        _native_init();
    }

    public void notifyPrepared() {
        if (mOnStateChangeListener != null) {
            mOnStateChangeListener.onPrepared();
        }
    }

    public void setDataSource(String dataSource) {
        _setDataSource(dataSource, mNativePtr);
    }

    public void prepare() {
        _prepare(mNativePtr);
    }

    public void prepareAsync() {
        _prepareAsync(mNativePtr);
    }

    public void start() {
        _start(mNativePtr);
    }

    public void pause() {
        _pause(mNativePtr);
    }

    public void resume() {
        _resume(mNativePtr);
    }

    public void seekTo(int time) {
        _seekTo(mNativePtr, time);
    }

    public void stop() {
        _stop(mNativePtr);
    }

    public void release() {
        _release(mNativePtr);
    }

    private void postEventFromNative(int what) {
        Message message = mEventHandler.obtainMessage(what);
        mEventHandler.sendMessage(message);
    }

    private void onTimeUpdate(int duration, double progress) {
        if (mOnTimeMetaDataAvailableListener != null) {
            if (mTimeMetaData == null) {
                mTimeMetaData = new TimeMetaData(duration, progress);
            } else {
                mTimeMetaData.setDuration(duration);
                mTimeMetaData.setProgressTime(progress);
            }
            mEventHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnTimeMetaDataAvailableListener.onTimeMetaDataAvailable(mTimeMetaData);
                }
            });
        }
    }

    private native void _setDataSource(String dataSource, long nativePtr);

    private native void _prepare(long nativePtr);

    private native void _prepareAsync(long nativePtr);

    private native void _native_init();

    private native void _start(long nativePtr);

    private native void _pause(long nativePtr);

    private native void _resume(long nativePtr);

    private native void _stop(long nativePtr);

    private native void _release(long nativePtr);

    private native void _seekTo(long nativePtr, int time);

    public void setOnPrepareListener(OnStateChangeListener onStateChangeListener) {
        this.mOnStateChangeListener = onStateChangeListener;
    }

    public interface OnStateChangeListener {
        void onPrepared();

        void onLoading();

        void onPlaying();

        void onFinished();
    }

    private OnTimeMetaDataAvailableListener mOnTimeMetaDataAvailableListener;

    public void setOnTimeMetaDataAvailableListener(OnTimeMetaDataAvailableListener onTimeMetaDataAvailableListener) {
        this.mOnTimeMetaDataAvailableListener = onTimeMetaDataAvailableListener;
    }

    public interface OnTimeMetaDataAvailableListener {
        void onTimeMetaDataAvailable(TimeMetaData timeMetaData);
    }

    public static class EventHandler extends Handler {
        private WeakReference<AudioPlayer> mAudioPlayerRef;

        public EventHandler(AudioPlayer audioPlayer) {
            this.mAudioPlayerRef = new WeakReference<>(audioPlayer);
        }

        @Override
        public void handleMessage(@NonNull Message msg) {
            AudioPlayer audioPlayer = mAudioPlayerRef.get();
            if (audioPlayer == null) {
                return;
            }
            OnStateChangeListener listener = audioPlayer.mOnStateChangeListener;
            if (listener == null) return;
            switch (msg.what) {
                case STATE_PREPARED:
                    listener.onPrepared();
                    break;
                case STATE_LOADING:
                    listener.onLoading();
                    break;
                case STATE_PLAYING:
                    listener.onPlaying();
                    break;
                case STATE_FINISHED:
                    listener.onFinished();
            }
        }
    }
}

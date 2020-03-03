package com.example.audioplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.example.audioplayer.bean.TimeMetaData;

import java.io.File;

public class MainActivity extends AppCompatActivity {


    private AudioPlayer audioPlayer;
    private TextView mAudioProgressTv;

    private static String[] urls = {
            "http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3",
            "http://mpge.5nd.com/2020/2020-2-29/95962/1.mp3",
            "http://mpge.5nd.com/2020/2020-2-21/95909/1.mp3",
            "http://mpge.5nd.com/2019/2019-12-16/95097/1.mp3"
    };
    private int index = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        String audioPath = new File(getExternalFilesDir(null), "audio.mp3").getAbsolutePath();
        audioPlayer = new AudioPlayer();
        audioPlayer.setDataSource(urls[0]);
        audioPlayer.setOnPrepareListener(new AudioPlayer.OnStateChangeListener() {
            @Override
            public void onPrepared() {
                Log.e("MediaPlayer", "---onPrepared");
                audioPlayer.start();
            }

            @Override
            public void onLoading() {
                Log.e("MediaPlayer", "---onLoading");
            }

            @Override
            public void onPlaying() {
                Log.e("MediaPlayer", "---onPlaying");
            }

            @Override
            public void onFinished() {
                Log.e("MediaPlayer", "---onFinished");
            }
        });
        audioPlayer.setOnTimeMetaDataAvailableListener(new AudioPlayer.OnTimeMetaDataAvailableListener() {
            @Override
            public void onTimeMetaDataAvailable(TimeMetaData timeMetaData) {
                mAudioProgressTv.setText(formatTime(timeMetaData.getProgressTime()) + "/" + formatTime(timeMetaData.getDuration()));
            }
        });
        mAudioProgressTv = findViewById(R.id.audio_progress_tv);
    }

    private String formatTime(double timeInSec) {
        int minute = (int) (timeInSec / 60);
        int sec = (int) (timeInSec % 60);
        return minute + ":" + sec;
    }

    public void start(View view) {
        audioPlayer.prepareAsync();
    }

    public void pause(View view) {
        audioPlayer.pause();
    }

    public void resume(View view) {
        audioPlayer.resume();
    }

    public String getSystemOperator() {
        String providersName = "";
        try {
            TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            if (telephonyManager == null) {
                return providersName;
            }

            @SuppressLint({"MissingPermission", "HardwareIds"})
            String IMSI = telephonyManager.getSubscriberId();
            if (!TextUtils.isEmpty(IMSI)) {
                if (IMSI.startsWith("46000") || IMSI.startsWith("46002")) {
                    providersName = "中国移动";
                } else if (IMSI.startsWith("46001")) {
                    providersName = "中国联通";
                } else if (IMSI.startsWith("46003")) {
                    providersName = "中国电信";
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return providersName;
    }

    public void seek(View view) {
        audioPlayer.seekTo(100);
    }

    public void stop(View view) {
        audioPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        audioPlayer.stop();
        audioPlayer.release();
    }

    public void next(View view) {
        audioPlayer.stop();
        index++;
        if (index >= urls.length) {
            index = 0;
        }
        audioPlayer.setDataSource(urls[index]);
        audioPlayer.prepare();
        audioPlayer.start();
    }

    public void last(View view) {
        audioPlayer.stop();
        index--;
        if (index <= 0) {
            index = 0;
        }
        audioPlayer.setDataSource(urls[index]);
        audioPlayer.prepare();
        audioPlayer.start();
    }
}

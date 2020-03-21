package com.example.audioplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.TextView;

import com.example.player.AudioPlayer;
import com.example.player.bean.TimeMetaData;
import com.example.player.enums.SoundChannel;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private AudioPlayer audioPlayer;
    private static Song[] songs = {
            new Song("第一首", "http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3"),
            new Song("第二首", "http://mpge.5nd.com/2020/2020-2-29/95962/1.mp3"),
            new Song("第三首", "http://mpge.5nd.com/2020/2020-2-21/95909/1.mp3"),
            new Song("第四首", "http://ngcdn001.cnr.cn/live/zgzs/index.m3u8")
    };
    private int index = 0;

    private TextView mSongNameTv;
    private CheckBox mPlayCb;
    private SeekBar mPlaySeekBar;
    private TextView mProgressTv;
    private TextView mTotalTimeTv;
    private SeekBar mVolumeSeekBar;
    private int mProgress;
    private boolean isSeeking;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
        initAudioPlayer();
    }

    private void initAudioPlayer() {
        String audioPath = new File(getExternalFilesDir(null), "audio.mp3").getAbsolutePath();
        audioPlayer = new AudioPlayer();
        audioPlayer.setDataSource(songs[0].url);
        audioPlayer.setOnPrepareListener(new AudioPlayer.OnStateChangeListener() {
            @Override
            public void onPrepared() {
                Log.e("MediaPlayer", "---onPrepared");
                audioPlayer.start();
                mPlayCb.setChecked(true);
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
                if (!isSeeking) {
                    int progress = (int) (timeMetaData.getProgressTime() / timeMetaData.getDuration() * 100);
                    mPlaySeekBar.setProgress(progress);
                }
                mProgressTv.setText(formatTime(timeMetaData.getProgressTime()));
                mTotalTimeTv.setText(formatTime(timeMetaData.getDuration()));
            }
        });
    }

    private void initView() {
        mSongNameTv = findViewById(R.id.song_name_tv);
        mPlaySeekBar = findViewById(R.id.play_progress_seekbar);
        mProgressTv = findViewById(R.id.progress_tv);
        mTotalTimeTv = findViewById(R.id.total_time_tv);
        mVolumeSeekBar = findViewById(R.id.volume_seekbar);
        mVolumeSeekBar.setProgress(50);
        mPlayCb = findViewById(R.id.play_checkbox);
        mPlaySeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    mProgress = progress;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                int targetTime = (int) ((mProgress * audioPlayer.getDuration() * 1.0f) / 100);
                audioPlayer.seekTo(targetTime);
                isSeeking = false;
            }
        });
        mVolumeSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                audioPlayer.setVolume(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        mPlayCb.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    audioPlayer.resume();
                } else {
                    audioPlayer.pause();
                }
            }
        });

        RadioGroup speedGroup = findViewById(R.id.speed_rg);
        speedGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                float speed = 1.0f;
                switch (checkedId) {
                    case R.id.speed_05:
                        speed = 0.5f;
                        break;
                    case R.id.speed_1:
                        speed = 1.0f;
                        break;
                    case R.id.speed_15:
                        speed = 1.5f;
                        break;
                    case R.id.speed_2:
                        speed = 2.0f;
                        break;
                }
                if (audioPlayer != null) {
                    audioPlayer.setSpeed(speed);
                }
            }
        });
        RadioGroup pitchGroup = findViewById(R.id.pitch_rg);
        pitchGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                float pitch = 1.0f;
                switch (checkedId) {
                    case R.id.pitch_05:
                        pitch = 0.5f;
                        break;
                    case R.id.pitch_1:
                        pitch = 1.0f;
                        break;
                    case R.id.pitch_15:
                        pitch = 1.5f;
                        break;
                    case R.id.pitch_2:
                        pitch = 2.0f;
                        break;
                }
                if (audioPlayer != null) {
                    audioPlayer.setPitch(pitch);
                }
            }
        });
    }

    private String formatTime(double timeInSec) {
        int minute = (int) (timeInSec / 60);
        int sec = (int) (timeInSec % 60);
        String minuteStr = minute < 10 ? "0" + minute : minute + "";
        String sedStr = sec < 10 ? "0" + sec : sec + "";
        return minuteStr + ":" + sedStr;
    }

    public void start(View view) {
        audioPlayer.prepareAsync();
    }

    public void stop(View view) {
        audioPlayer.stop();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
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
        if (index >= songs.length) {
            index = 0;
        }
        playSong(songs[index]);
    }

    public void last(View view) {
        audioPlayer.stop();
        index--;
        if (index < 0) {
            index = songs.length - 1;
        }
        playSong(songs[index]);
    }

    private void playSong(Song song) {
        if (song == null) return;

        mSongNameTv.setText(song.name);
        audioPlayer.setDataSource(song.url);
        audioPlayer.prepareAsync();
    }

    public void setLeftMute(View view) {
        audioPlayer.setSoundChannel(SoundChannel.LEFT);
    }

    public void setCenterMute(View view) {
        audioPlayer.setSoundChannel(SoundChannel.CENTER);
    }

    public void setRightMute(View view) {
        audioPlayer.setSoundChannel(SoundChannel.RIGHT);
    }
}

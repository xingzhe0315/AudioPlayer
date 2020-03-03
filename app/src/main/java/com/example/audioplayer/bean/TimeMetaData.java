package com.example.audioplayer.bean;

public class TimeMetaData {
    private int duration;
    private double progressTime;

    public TimeMetaData(int duration, double progressTime) {
        this.duration = duration;
        this.progressTime = progressTime;
    }

    public int getDuration() {
        return duration;
    }

    public double getProgressTime() {
        return progressTime;
    }

    public void setDuration(int duration) {
        this.duration = duration;
    }

    public void setProgressTime(double progressTime) {
        this.progressTime = progressTime;
    }
}

package com.example.audioplayer.enums;

public enum SoundChannel {
    CENTER(0), LEFT(1), RIGHT(2);
    int value;

    SoundChannel(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}

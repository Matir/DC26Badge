package com.attackercommunity.acdcbadge;

// Constants are in terms of DISP_UPDATE_FREQUENCY_MS in led_display.c in the firmware.
// Currently 50ms intervals

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public enum MessageSpeed {
    MSG_SPEED_SLOWEST (64, "Slowest"),
    MSG_SPEED_SLOW (32, "Slow"),
    MSG_SPEED_MEDIUM (16, "Medium"),
    MSG_SPEED_FAST (8, "Fast"),
    MSG_SPEED_FASTEST (4, "Fastest");

    // Size of underlying representation in bytes
    public static final short SIZE = 2;

    private short speed;
    private String speedName;

    MessageSpeed(int speed, String speedName) {
        this.speed = (short)speed;
        this.speedName = speedName;
    }

    public String toString() {
        return this.speedName;
    }

    public short encode() {
        return speed;
    }

    public static MessageSpeed fromSpeed(short val) {
        for (MessageSpeed s: MessageSpeed.values()) {
            if (s.speed == val) {
                return s;
            }
        }
        return null;
    }

    public static MessageSpeed fromNearest(short val) {
        for (MessageSpeed s: MessageSpeed.values()) {
            if (s.speed < val)
                return s;
        }
        return MSG_SPEED_MEDIUM;
    }

    public static List<MessageSpeed> asList() {
        return Collections.unmodifiableList(Arrays.asList(MessageSpeed.values()));
    }
}

package com.attackercommunity.acdcbadge;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

// The different badge modes
public enum MessageMode {
    MSG_STATIC (0, "Static"),
    MSG_SCROLL (1, "Scroll"),
    MSG_REPLACE (2, "Replace"),
    MSG_WARGAMES (3, "Wargames"),
    MSG_SCROLL_LOOP (4, "Loop");

    private final byte modeId;
    private final String modeName;

    // Size of underlying representation in bytes
    public static final short SIZE = 1;

    MessageMode(int val, String modeName) {
        this.modeId = (byte)val;
        this.modeName = modeName;
    }

    @Override
    public String toString() {
        return modeName;
    }

    public byte encode() {
        return modeId;
    }

    public static MessageMode fromByte(byte val) {
        for (MessageMode m: MessageMode.values()) {
            if (m.modeId == val)
                return m;
        }
        return null;
    }

    public static MessageMode fromString(String val) {
        for (MessageMode m: MessageMode.values()) {
            if (m.modeName.equals(val))
                return m;
        }
        return null;
    }

    public static List<MessageMode> asList() {
        return Collections.unmodifiableList(Arrays.asList(MessageMode.values()));
    }
}
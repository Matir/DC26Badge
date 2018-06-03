package com.attackercommunity.acdcbadge;

import java.util.UUID;

public final class Constants {
    public static final UUID BadgeServiceUUID = UUID.fromString("00004141-e87e-4706-acf7-8c633c19c4d5");
    public static final UUID DisplayOnOffUUID = UUID.fromString("00004242-e87e-4706-acf7-8c633c19c4d5");
    public static final UUID BadgeIndexUUID = UUID.fromString("00004343-e87e-4706-acf7-8c633c19c4d5");
    public static final UUID DisplayBrightnessUUID = UUID.fromString("00004444-e87e-4706-acf7-8c633c19c4d5");
    public static final UUID MessageUUID = UUID.fromString("00004545-e87e-4706-acf7-8c633c19c4d5");
    public static final long ScanDelayMillis = 3000;  // Time to batch up results
    public static final long ScanTimeMillis = 15000;  // Total time before stopping scan
    public static final String BLEDevMessage = "com.attackercommunity.acdcbadge.BLE_DEVICE";
    public static final int MessageMaxLength = 35; // Must be kept in sync with firmware!
    public static final int MaxBrightness = 16;  // Maximum screen brightness
    public static final boolean PermitUnknownRates = true; // Permit unknown rates coming from firmware
}

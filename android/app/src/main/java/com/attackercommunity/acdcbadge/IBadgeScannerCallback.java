package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;

public interface IBadgeScannerCallback {
    void onScanFailed(int errorCode);
    void onBLEDevice(BluetoothDevice device, String name);
    void onScanStopped();
}

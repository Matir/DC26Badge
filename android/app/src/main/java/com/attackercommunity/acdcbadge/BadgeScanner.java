package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.os.Handler;
import android.os.ParcelUuid;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class BadgeScanner implements IBadgeScanner {
    private static final String TAG = "BadgeScanner";
    private BadgeScannerScanCallback mCallback = null;

    // Build a scan filter for our service
    private List<ScanFilter> buildScanFilters() {
        ParcelUuid puuid = ParcelUuid.fromString(Constants.BadgeServiceUUID);
        ScanFilter serviceUUIDFilter = (new ScanFilter.Builder()).setServiceUuid(puuid).build();
        ArrayList<ScanFilter> rv = new ArrayList<>();
        rv.add(serviceUUIDFilter);
        return rv;
    }

    public void scan(Context ctx, IBadgeScannerCallback resultCallback) {
        if (mCallback != null) {
            Log.e(TAG, "Attempted to Start BLE Scan with another in progress.");
            return;
        }
        BluetoothManager manager = (BluetoothManager) ctx.getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager.getAdapter();
        final BluetoothLeScanner scanner = adapter.getBluetoothLeScanner();
        ScanSettings.Builder ssBuilder = new ScanSettings.Builder();
        ssBuilder.setReportDelay(Constants.ScanDelayMillis);
        ssBuilder.setScanMode(ScanSettings.SCAN_MODE_BALANCED);
        Log.i(TAG, "Starting BLE Scan");
        mCallback = new BadgeScannerScanCallback(resultCallback);
        scanner.startScan(
                buildScanFilters(),
                ssBuilder.build(),
                mCallback);
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "Stopping BLE Scan");
                scanner.stopScan(mCallback);
                mCallback.onScanStopped();
                mCallback = null;
            }
        }, Constants.ScanTimeMillis);
    }

    private class BadgeScannerScanCallback extends ScanCallback {
        private IBadgeScannerCallback mCallback;

        BadgeScannerScanCallback(IBadgeScannerCallback resultCallback) {
            mCallback = resultCallback;
        }

        // Failures get passed on
        @Override
        public void onScanFailed(int errorCode) {
            Log.e(TAG, "Scan failed with errorCode: " + errorCode);
            mCallback.onScanFailed(errorCode);
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            Log.d(TAG, "Received batch scan results.");
            for (ScanResult result: results) {
                handleScanResult(result);
            }
        }

        @Override
        public void onScanResult(int unused_callbackType, ScanResult result) {
            Log.d(TAG, "Received scan result.");
            handleScanResult(result);
        }

        private void onScanStopped() {
            mCallback.onScanStopped();
        }

        private void handleScanResult(ScanResult result) {
            BluetoothDevice dev = result.getDevice();
            Log.d(TAG, "Scan saw device with name " + dev.getName() + " " + dev.getAddress());
            mCallback.onBLEDevice(dev);
        }
    }
}

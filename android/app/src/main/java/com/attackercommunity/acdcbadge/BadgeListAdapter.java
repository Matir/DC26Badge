package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.graphics.Typeface;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.ListIterator;
import java.util.Set;

public class BadgeListAdapter extends RecyclerView.Adapter<BadgeListAdapter.ViewHolder> {
    private static final String TAG = "BadgeListAdapter";
    private ArrayList<BluetoothDevice> mDevices;
    private Set<String> mDeviceIds;

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final DeviceDisplayLayout mDisplay;
        public ViewHolder(DeviceDisplayLayout display) {
            super(display);
            mDisplay = display;
        }
    }

    // Construct with empty list
    public BadgeListAdapter() {
        mDevices = new ArrayList<>();
        mDeviceIds = new HashSet<>();
    }

    @Override
    public BadgeListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        // Loads and creates a new layout from XML
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        DeviceDisplayLayout layout = (DeviceDisplayLayout)inflater.inflate(R.layout.badge_device, parent, false);
        layout.setupView();
        return new ViewHolder(layout);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        BluetoothDevice dev;
        synchronized (this) {
            if ((position < 0) || (position >= mDeviceIds.size())) {
                Log.e(TAG, "Requested device outside range.");
                return;
            }
            dev = mDevices.get(position);
        }
        holder.mDisplay.setAddress(dev.getAddress());
        holder.mDisplay.setDeviceName(dev.getName());
    }

    @Override
    public int getItemCount() {
        return mDevices.size();
    }

    public void refreshView(Context ctx) {
        synchronized (this) {
            mDeviceIds.clear();
            mDevices.clear();
        }
        notifyDataSetChanged();
        // Trigger a scan
        // TODO: figure out a way to mock in emulator?
        IBadgeScanner scanner = new BadgeScanner();
        scanner.scan(ctx, this.new ScannerCallback());
    }

    private int compareDevices(@NonNull BluetoothDevice a, @NonNull BluetoothDevice b) {
        // Returns <0 if a < b, 0 if a == b, >0 if a > b
        // All bonded devices sort before all unbonded devices.
        if (isBonded(a) && !isBonded(b))
            return -1;
        if (!isBonded(a) && isBonded(b))
            return 1;
        return a.getAddress().compareTo(b.getAddress());
    }

    private boolean isBonded(@NonNull BluetoothDevice dev) {
        return dev.getBondState() == BluetoothDevice.BOND_BONDED;
    }

    private class ScannerCallback implements IBadgeScannerCallback {
        public void onScanFailed(int errorCode) {}
        public void onScanStopped() {}
        public void onBLEDevice(BluetoothDevice device){
            synchronized(BadgeListAdapter.this) {
                if (mDeviceIds.contains(device.getAddress())) {
                    // Already have the device.
                    return;
                }
                mDeviceIds.add(device.getAddress());
                ListIterator<BluetoothDevice> it = mDevices.listIterator();
                while(it.hasNext()) {
                    if (compareDevices(device, (BluetoothDevice) it.next()) < 0) {
                        it.previous();
                        it.add(device);
                        notifyItemInserted(it.previousIndex());
                        return;
                    }
                }
                // Did not find a position, goes at the end
                mDevices.add(device);
                notifyItemInserted(mDeviceIds.size()-1);
            } // synchronized block
        }
    }

    public static class DeviceDisplayLayout extends LinearLayout {
        private Context mCtx;
        private TextView mNameView;
        private TextView mAddressView;

        public DeviceDisplayLayout(Context ctx) {
            super(ctx);
            mCtx = ctx;
        }

        public DeviceDisplayLayout(Context ctx, AttributeSet attrs) {
            super(ctx, attrs);
            mCtx = ctx;
        }

        public DeviceDisplayLayout(Context ctx, AttributeSet attrs, int defStyleAttr) {
            super(ctx, attrs, defStyleAttr);
            mCtx = ctx;
        }

        public DeviceDisplayLayout(Context ctx, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
            super(ctx, attrs, defStyleAttr, defStyleRes);
            mCtx = ctx;
        }

        public void setDeviceName(String name) {
            mNameView.setText(name);
        }

        public void setAddress(String address) {
            mAddressView.setText(address);
        }

        public void setupView() {
            TextView v = findViewById(R.id.deviceIconView);
            v.setGravity(Gravity.CENTER);
            Typeface font = Typeface.createFromAsset(mCtx.getAssets(), "fonts/fontawesome.otf");
            v.setTypeface(font);
            mNameView = (TextView) findViewById(R.id.badge_name);
            mAddressView = (TextView) findViewById(R.id.badge_address);
        }
    }
}

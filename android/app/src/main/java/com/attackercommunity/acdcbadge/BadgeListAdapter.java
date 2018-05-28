package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.graphics.Typeface;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.ListIterator;
import java.util.Set;

/*
This class has become way too coupled to the UI.  This is what happens when your first real
Android app has significant complexity.
*/
public class BadgeListAdapter extends RecyclerView.Adapter<BadgeListAdapter.ViewHolder> {
    private static final String TAG = "BadgeListAdapter";
    private ArrayList<BluetoothDevice> mDevices;
    private Set<String> mDeviceIds;
    private View mContainingView = null;
    private boolean mIsScanning = false;

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final DeviceDisplayLayout mDisplay;
        public BluetoothDevice mDevice;
        public ViewHolder(DeviceDisplayLayout display) {
            super(display);
            mDisplay = display;
            display.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // handle click event
                    Log.i(TAG, "Clicked for device " + mDevice.getAddress());
                }
            });
        }
    }

    // Construct with empty list
    public BadgeListAdapter() {
        super();
        mDevices = new ArrayList<>();
        mDeviceIds = new HashSet<>();
    }

    public BadgeListAdapter(View view) {
        this();
        mContainingView = view;
        Log.i(TAG, "mContainingView is " + mContainingView.toString());
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
        position /= 2;
        BluetoothDevice dev;
        synchronized (this) {
            if ((position < 0) || (position >= mDeviceIds.size())) {
                Log.e(TAG, "Requested device outside range.");
                return;
            }
            dev = mDevices.get(position);
        }
        holder.mDevice = dev;
        holder.mDisplay.setAddress(dev.getAddress());
        holder.mDisplay.setDeviceName(dev.getName());
    }

    @Override
    public int getItemCount() {
        //TODO: remove debugging 2x size
        return mDevices.size()*2;
    }

    public void refreshView(Context ctx) {
        synchronized (this) {
            if (mIsScanning) {
                Log.i(TAG, "Scan already in progress.");
                return;
            }
            mIsScanning = true;
            showHideProgress(true);
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

    private void showHideProgress(final boolean show) {
        if (mContainingView == null)
            return;
        mContainingView.post(new Runnable() {
            @Override
            public void run() {
                View view = mContainingView.findViewById(R.id.scanning_loading);
                if (view == null)
                    return;
                if (show) {
                    Log.d(TAG, "Showing spinner.");
                    view.setVisibility(View.VISIBLE);
                } else {
                    Log.d(TAG, "Hiding spinner.");
                    view.setVisibility(View.GONE);
                }
            }
        });
    }

    private class ScannerCallback implements IBadgeScannerCallback {
        public void onScanFailed(int errorCode) {
            if (mContainingView == null)
                return;
            Snackbar.make(mContainingView, "Error scanning for BLE devices.", Snackbar.LENGTH_LONG);
        }

        public void onScanStopped() {
            synchronized (BadgeListAdapter.this) {
                mIsScanning = false;
            }
            showHideProgress(false);
        }

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

    // TODO: no reason for this to be an inner class
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
            Typeface font = FontFoundry.Get().getTypeface(mCtx, "fontawesome-solid.otf");
            v.setTypeface(font);
            mNameView = (TextView) findViewById(R.id.badge_name);
            mAddressView = (TextView) findViewById(R.id.badge_address);
            // Force redraw
            invalidate();
        }
    }
}

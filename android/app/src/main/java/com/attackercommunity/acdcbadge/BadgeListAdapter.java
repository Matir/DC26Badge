package com.attackercommunity.acdcbadge;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
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
import java.util.HashMap;
import java.util.HashSet;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;

/*
This class has become way too coupled to the UI.  This is what happens when your first real
Android app has significant complexity.
*/
public class BadgeListAdapter extends RecyclerView.Adapter<BadgeListAdapter.ViewHolder> {
    private static final String TAG = "BadgeListAdapter";
    private ArrayList<BluetoothDevice> mDevices;
    private Set<String> mDeviceIds;
    private Map<String, String> mDeviceNames = new HashMap<>();
    private Activity mContainingActivity = null;
    private View mContainingView = null;
    private boolean mIsScanning = false;

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final DeviceDisplayLayout mDisplay;
        public BluetoothDevice mDevice;
        public ViewHolder(DeviceDisplayLayout display, final Activity activity) {
            super(display);
            mDisplay = display;
            display.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // handle click event
                    Log.i(TAG, "Clicked for device " + mDevice.getAddress());
                    Intent intent = new Intent(v.getContext(), BadgeSetupActivity.class);
                    intent.putExtra(Constants.BLEDevMessage, mDevice);
                    activity.startActivity(intent);
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

    public BadgeListAdapter(View view, Activity activity) {
        this();
        mContainingView = view;
        mContainingActivity = activity;
        Log.i(TAG, "mContainingView is " + mContainingView.toString());
    }

    @Override
    public BadgeListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        // Loads and creates a new layout from XML
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        DeviceDisplayLayout layout = (DeviceDisplayLayout)inflater.inflate(R.layout.badge_device, parent, false);
        layout.setupView();
        return new ViewHolder(layout, mContainingActivity);
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
        holder.mDevice = dev;
        holder.mDisplay.setAddress(dev.getAddress());
        String name = dev.getName();
        if (name == null) {
            name = mDeviceNames.get(dev.getAddress());
        }
        if (name == null) {
            name = "DC26 Badge";
        }
        holder.mDisplay.setDeviceName(name);
        holder.mDisplay.setBonded(dev.getBondState() == BluetoothDevice.BOND_BONDED);
    }

    @Override
    public int getItemCount() {
        return mDevices.size();
    }

    public void refreshView(Context ctx) {
        synchronized (this) {
            if (mIsScanning) {
                Log.i(TAG, "Scan already in progress.");
                return;
            }
            mIsScanning = true;
            showHideProgress(true, false);
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

    private void showHideProgress(final boolean showSpinner, final boolean showNoBadges) {
        if (mContainingView == null)
            return;
        mContainingView.post(new Runnable() {
            @Override
            public void run() {
                View view = mContainingView.findViewById(R.id.scanning_loading);
                if (view == null)
                    return;
                if (showSpinner) {
                    Log.d(TAG, "Showing spinner.");
                    view.setVisibility(View.VISIBLE);
                } else {
                    Log.d(TAG, "Hiding spinner.");
                    view.setVisibility(View.GONE);
                }
                view = mContainingView.findViewById(R.id.no_badges_text);
                if (view == null)
                    return;
                if (showNoBadges) {
                    view.setVisibility(View.VISIBLE);
                } else {
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
            boolean showNoBadges = (getItemCount() == 0);
            showHideProgress(false, showNoBadges);
        }

        public void onBLEDevice(BluetoothDevice device, String name){
            final String address = device.getAddress();
            synchronized(BadgeListAdapter.this) {
                if (name != null) {
                    mDeviceNames.put(address, name);
                }
                if (mDeviceIds.contains(address)) {
                    // Already have the device.
                    BluetoothDevice oldDevice = null;
                    for (BluetoothDevice testDevice : mDevices) {
                        if (testDevice.getAddress().equals(device.getAddress())) {
                            oldDevice = testDevice;
                            break;
                        }
                    }
                    if (oldDevice != null) {
                        Log.d(TAG, "Device already existed, comparing (old vs new).");
                        Log.d(TAG, "Address: " + oldDevice.getAddress() + " " + device.getAddress());
                        Log.d(TAG, "Name: " + oldDevice.getName() + " " + device.getName());
                        Log.d(TAG, "Bond state: " + oldDevice.getBondState() + " " + device.getBondState());
                        Log.d(TAG, "Type: " + oldDevice.getType() + " " + device.getType());
                        Log.d(TAG, "Hash Code: " + oldDevice.hashCode() + " " + device.hashCode());
                    }
                    return;
                }
                mDeviceIds.add(address);
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
        private TextView mBondedView;

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

        public void setBonded(boolean bonded) {
            if (bonded) {
                mBondedView.setVisibility(VISIBLE);
            } else {
                mBondedView.setVisibility(GONE);
            }
        }

        public void setupView() {
            mNameView = (TextView) findViewById(R.id.badge_name);
            mAddressView = (TextView) findViewById(R.id.badge_address);
            mBondedView = (TextView) findViewById(R.id.icon_lock);
            // Force redraw
            invalidate();
        }
    }
}

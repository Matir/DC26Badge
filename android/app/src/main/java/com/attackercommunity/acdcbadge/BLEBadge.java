package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.annotation.NonNull;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.UUID;

// Abstracts a single badge and handles BLE interfacing.
// TODO: consider making a Service?
public final class BLEBadge {
    private static final String TAG = "BLEBadge";

    private final Context mContext;
    private final BluetoothDevice mDevice;
    private BluetoothGatt mBluetoothGatt = null;
    private BluetoothGattService mBadgeService = null;
    private BLEBadgeUpdateNotifier mNotifier = null;
    private int mPendingCharacteristics = 0;
    private GattQueue mQueue = null;

    // Data from the badge itself
    private boolean mDisplayEnabled = false;
    private byte mBrightness = 0;
    private final List<BLEBadgeMessage> mMessages = new ArrayList<>();

    public BLEBadge(@NonNull Context ctx, @NonNull BluetoothDevice device) {
        mContext = ctx;
        mDevice = device;
        receiverSetup();
    }

    // Properties

    // Get the name of the device.
    public String getName() {
        return mDevice.getName();
    }

    // Get the address of the device.
    public String getAddress() {
        return mDevice.getAddress();
    }

    // Is the display enabled?
    public boolean getDisplayEnabled() {
        return mDisplayEnabled;
    }

    // Enable/disable the display
    public void setDisplayEnabled(boolean value) {
        mDisplayEnabled = value;
        BluetoothGattCharacteristic dispChar = mBadgeService.getCharacteristic(
                Constants.DisplayOnOffUUID);
        if (dispChar == null) {
            Log.e(TAG, "No characteristic found in setBrightness.");
            return;
        }
        int byVal = value ? 1 : 0;
        dispChar.setValue(byVal, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        mQueue.add(new GattQueueOperation(GattOperation.WRITE, dispChar));
    }

    // Get the brightness
    public byte getBrightness() {
        return mBrightness;
    }

    // Change the brightness
    public void setBrightness(byte newVal) throws BLEBadgeException {
        if (newVal > Constants.MaxBrightness)
            throw new BLEBadgeException("Brightness exceeds maximum value.");
        BluetoothGattCharacteristic bright = mBadgeService.getCharacteristic(
                Constants.DisplayBrightnessUUID);
        if (bright == null) {
            Log.e(TAG, "No characteristic found in setBrightness.");
            return;
        }
        bright.setValue(newVal, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        mQueue.add(new GattQueueOperation(GattOperation.WRITE, bright));
    }

    // Get the messages
    // The list is copied to allow the frontend to independently operate on the messages
    public List<BLEBadgeMessage> getMessages() {
        synchronized (this.mMessages) {
            ArrayList<BLEBadgeMessage> newList = new ArrayList<>(mMessages.size());
            newList.addAll(mMessages);
            return Collections.unmodifiableList(newList);
        }
    }

    // Connect to remote
    // Returns true if connected or connecting, false if not yet bonded
    public boolean connect() {
        if (connected())
            return true;
        if (ensureBonded()) {
            mBluetoothGatt = mDevice.connectGatt(mContext, true, mGattCallback);
            mQueue = new GattQueue(mBluetoothGatt);
            return true;
        }
        return false;
    }

    // Are we currently connected?
    public boolean connected() {
        BluetoothManager svc = (BluetoothManager)mContext.getSystemService(
                Context.BLUETOOTH_SERVICE);
        return (svc.getConnectionState(mDevice, BluetoothGatt.GATT)
                == BluetoothProfile.STATE_CONNECTED);
    }

    // Close connection
    public void close() {
        if (mBluetoothGatt == null)
            return;
        mBluetoothGatt.close();
        mBluetoothGatt = null;
        mBadgeService = null;

        Log.d(TAG, "Unregistering intent receiver.");
        mContext.unregisterReceiver(mBroadcastReceiver);
    }

    public void setUpdateNotifier(BLEBadgeUpdateNotifier notifier) {
        mNotifier = notifier;
    }

    // Abstract class to implement to receive notification updates
    public static abstract class BLEBadgeUpdateNotifier {
        abstract void onChanged(BLEBadge badge);
        abstract void onError(BLEBadge badge, String error);
    }


    // Notify of changes
    private void notifyChanged() {
        if (mNotifier == null)
            return;
        mNotifier.onChanged(this);
    }

    // Notify of errors
    private void notifyError(String error) {
        if (mNotifier == null)
            return;
        mNotifier.onError(this, error);
    }

    // Setup BLE events
    private void receiverSetup() {
        Log.d(TAG, "Registering intent receiver.");
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        mContext.registerReceiver(mBroadcastReceiver, filter);
    }

    // Check if we are bonded, and if not, start bonding
    private boolean ensureBonded() {
        if (mDevice.getBondState() == BluetoothDevice.BOND_BONDED) {
            return true;
        }
        boolean rv = mDevice.createBond();
        if (!rv) {
            Log.e(TAG, "Error requesting to create bond!");
        }
        return false;
    }

    // Updates all the characteristics
    private void updateCharacteristics() {
        if (!connected()) {
            Log.e(TAG, "Cannot update characteristics when not connected.");
            return;
        }
        List<BluetoothGattCharacteristic> chars = mBadgeService.getCharacteristics();
        synchronized (this) {
            mPendingCharacteristics = chars.size();
        }
        for(BluetoothGattCharacteristic item : chars) {
            Log.d(TAG, "Updating " + item.getUuid());
            mQueue.add(new GattQueueOperation(GattOperation.READ, item));
        }
    }

    // Update the internal badge state
    private void updateState() {
        // Update display state
        BluetoothGattCharacteristic onOff = mBadgeService.getCharacteristic(
                Constants.DisplayOnOffUUID);
        if (onOff != null) {
            Integer intVal = onOff.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
            mDisplayEnabled = (intVal == 1);
        } else {
            Log.w(TAG, "Could not find ON/OFF Characteristic in updateState.");
        }
        // Update display brightness
        BluetoothGattCharacteristic brightness = mBadgeService.getCharacteristic(
                Constants.DisplayBrightnessUUID);
        if (brightness != null) {
            Integer intVal = brightness.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
            mBrightness = intVal.byteValue();
        } else {
            Log.w(TAG, "Could not find brightness characteristic in updateState.");
        }
        // Update messages
        UUID msgUUID = Constants.MessageUUID;
        List<BLEBadgeMessage> messages = new ArrayList<>();
        for (BluetoothGattCharacteristic item : mBadgeService.getCharacteristics()) {
            if (!item.getUuid().equals(msgUUID))
                continue;
            try {
                BLEBadgeMessage newMessage = BLEBadgeMessage.fromCharacteristic(item);
                messages.add(newMessage);
            } catch(BLEBadgeException ex) {
                Log.e(TAG, "Unable to parse BLE Badge Message", ex);
            }
        }
        synchronized (this.mMessages) {
            mMessages.clear();
            mMessages.addAll(messages);
        }
        // Finally notify that state has changed
        notifyChanged();
    }

    // The different badge modes
    public enum MessageMode {
        MSG_STATIC (0),
        MSG_SCROLL (1),
        MSG_REPLACE (2),
        MSG_WARGAMES (3);

        private final byte modeId;
        private static final String[] modeNames = new String[]{
                "Static", "Scroll", "Replace", "Wargames"};

        MessageMode(int val) {
            modeId = (byte)val;
        }

        @Override
        public String toString() {
            try {
                return modeNames[modeId];
            } catch (ArrayIndexOutOfBoundsException ex) {
                return "UNKNOWN";
            }
        }

        public static MessageMode fromByte(byte val) {
            for (MessageMode m: MessageMode.values()) {
                if (m.modeId == val)
                    return m;
            }
            return null;
        }

        public static List<String> getNames() {
            return Collections.unmodifiableList(Arrays.asList(modeNames));
        }
    }

    // Represents a single badge message
    public static final class BLEBadgeMessage {

        private MessageMode mMode;
        private short mRate;
        private String mText;
        private boolean changed = false;

        private BLEBadgeMessage(MessageMode mode, short rate, String text) {
            mMode = mode;
            mRate = rate;
            mText = text;
        }

        public static BLEBadgeMessage fromCharacteristic(BluetoothGattCharacteristic characteristic)
                throws BLEBadgeException {
            return fromBytes(characteristic.getValue());
        }

        public static BLEBadgeMessage fromBytes(byte[] value) throws BLEBadgeException {
            ByteBuffer readBuffer = ByteBuffer.wrap(value);
            readBuffer.order(ByteOrder.LITTLE_ENDIAN);
            MessageMode mode = MessageMode.fromByte(readBuffer.get());
            if (mode == null) {
                throw new BLEBadgeException("Unknown message mode!");
            }
            short rate = readBuffer.getShort();
            int offset = readBuffer.position();
            int length = value.length - offset;
            for(int i = offset; i < value.length; i ++) {
                if (value[i] == (byte)0) {
                    length = i - offset;
                    break;
                }
            }
            String text = new String(value, offset, length, Charset.forName("US-ASCII"));
            return new BLEBadgeMessage(mode, rate, text);
        }

        public String getText() {
            return mText;
        }

        public void setText(String text) throws BLEBadgeException {
            if (text.length() > Constants.MessageMaxLength) {
                Log.e(TAG, "Attempting to set too long message!");
                throw new BLEBadgeException(
                        "Message is limited to " + Constants.MessageMaxLength + " characters.");
            }
            mText = text;
            changed = true;
        }

        public short getRate() {
            return mRate;
        }

        public void setRate(short rate) {
            mRate = rate;
            changed = true;
        }

        public MessageMode getMode() {
            return mMode;
        }

        public void setMode(MessageMode mode) {
            mMode = mode;
            changed = true;
        }

        public boolean hasChanges() {
            return changed;
        }
    }


    // For notifications about bonding
    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Received intent: " + intent.getAction());
            if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) {
                int newState = intent.getIntExtra(
                        BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.BOND_NONE);
                BluetoothDevice affected = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (newState == BluetoothDevice.BOND_BONDED && affected.equals(mDevice)) {
                    connect();
                }
            }
        }
    };

    // Various GATT Callbacks
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);
            Log.d(TAG, "GATT State changed to " + bluetoothGattStateString(newState));
            if (newState == BluetoothGatt.STATE_CONNECTED) {
                if (!gatt.discoverServices()) {
                    Log.e(TAG, "Error requesting service discovery.");
                }
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            super.onServicesDiscovered(gatt, status);
            Log.d(TAG, "Services discovered, updating characteristics.");
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.e(TAG, "Error discovering services");
                return;
            }
            UUID badgeServiceUUID = Constants.BadgeServiceUUID;
            mBadgeService = gatt.getService(badgeServiceUUID);
            if (mBadgeService == null) {
                Log.e(TAG, "The Badge Service was not offered by this device!");
                return;
            }
            updateCharacteristics();
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicRead(gatt, characteristic, status);
            Log.d(TAG, "Updated characteristic via read.");
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.e(TAG, "Error reading characteristic: " + status);
                return;
            }
            boolean allUpdated = false;
            synchronized (BLEBadge.this) {
                mPendingCharacteristics--;
                Log.d(TAG, "Pending: " + mPendingCharacteristics);
                allUpdated = (mPendingCharacteristics == 0);
            }
            if (allUpdated)
                updateState();
            mQueue.executeNext();
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicWrite(gatt, characteristic, status);
            Log.i(TAG, "Characteristic write completed with status " + status);
            mQueue.executeNext();
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
            super.onReliableWriteCompleted(gatt, status);
            Log.i(TAG, "Reliable write completed with status " + status);
            if (status != BluetoothGatt.GATT_SUCCESS) {
                notifyError("Failed to write changes to device.");
            }
            mQueue.executeNext();
        }
    };

    private enum GattOperation {
        READ, WRITE
    }

    private static final class GattQueueOperation {
        private final GattOperation op;
        private final BluetoothGattCharacteristic target;

        public GattQueueOperation(GattOperation op, BluetoothGattCharacteristic target) {
            this.op = op;
            this.target = target;
        }
    }

    private static final class GattQueue {
        private final Queue<GattQueueOperation> queue = new LinkedList<>();
        private final BluetoothGatt mGatt;
        private boolean pending = false; // Is an operation currently pending?

        public GattQueue(BluetoothGatt gatt){
            mGatt = gatt;
        }

        public void add(GattQueueOperation op) {
            synchronized (this) {
                if (!pending) {
                    pending = true;
                    executeOp(op);
                    return;
                }
                queue.add(op);
            }
        }

        public Boolean executeNext() {
            GattQueueOperation op;
            synchronized (this) {
                op = queue.poll();
                if (op == null) {
                    pending = false;
                    return null;
                }
                pending = true;
            }
            return executeOp(op);
        }

        private boolean executeOp(GattQueueOperation op) {
            if (op.op == GattOperation.READ) {
                return mGatt.readCharacteristic(op.target);
            } else if (op.op == GattOperation.WRITE) {
                mGatt.beginReliableWrite();
                boolean rv = mGatt.writeCharacteristic(op.target);
                mGatt.executeReliableWrite();
                return rv;
            }
            Log.e(TAG, "Unknown operation!");
            return false;
        }
    }

    // Just for making things pretty
    @NonNull private static String bluetoothGattStateString(int state) {
        switch(state) {
            case BluetoothGatt.STATE_CONNECTED:
                return "Connected";
            case BluetoothGatt.STATE_CONNECTING:
                return "Connecting";
            case BluetoothGatt.STATE_DISCONNECTED:
                return "Disconnected";
            case BluetoothGatt.STATE_DISCONNECTING:
                return "Disconnecting";
            default:
                return "UNKNOWN";
        }
    }

    public static class BLEBadgeException extends Exception {
        public BLEBadgeException() {
        }

        public BLEBadgeException(String message) {
            super(message);
        }
    }
}

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

import java.io.UnsupportedEncodingException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.ArrayList;
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
    private byte mCurrentMessage = 0;
    private final List<BLEBadgeMessage> mMessages = new ArrayList<>();
    private final List<BluetoothGattCharacteristic> mMessageChars = new ArrayList<>();

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

    // Change the name of the device
    public void setName(String newName) {
        if (mBluetoothGatt == null || mQueue == null || !connected()) {
            Log.e(TAG, "Not connected, can't change name!");
            return;
        }
        BluetoothGattService gapService = mBluetoothGatt.getService(
                Constants.GenericAccessServiceUUID);
        BluetoothGattCharacteristic nameChar = gapService.getCharacteristic(
                Constants.DeviceNameUUID);
        nameChar.setValue(newName);
        mQueue.add(GattQueueOperation.Write(nameChar));
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
        mQueue.add(GattQueueOperation.Write(dispChar));
    }

    // Get the brightness
    public byte getBrightness() {
        return mBrightness;
    }

    // Change the brightness
    public void setBrightness(byte newVal) throws BLEBadgeException {
        if (newVal > Constants.MaxBrightness)
            throw new BLEBadgeException("Brightness exceeds maximum value.");
        mBrightness = newVal;
        BluetoothGattCharacteristic bright = mBadgeService.getCharacteristic(
                Constants.DisplayBrightnessUUID);
        if (bright == null) {
            Log.e(TAG, "No characteristic found in setBrightness.");
            return;
        }
        bright.setValue(newVal, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        mQueue.add(GattQueueOperation.Write(bright));
    }

    public byte getCurrentMessage() {
        return mCurrentMessage;
    }

    public void setCurrentMessage(byte newVal) throws BLEBadgeException {
        if (newVal < 0 || newVal > mMessages.size()) {
            throw new BLEBadgeException("New Message Out of Range");
        }
        mCurrentMessage = newVal;
        BluetoothGattCharacteristic index = mBadgeService.getCharacteristic(
                Constants.BadgeIndexUUID);
        if (index == null) {
            Log.e(TAG, "No characteristic found in setCurrentMessage.");
            return;
        }
        index.setValue(newVal, BluetoothGattCharacteristic.FORMAT_SINT8, 0);
        mQueue.add(GattQueueOperation.Write(index));
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

    // Save messages that have been changed
    public void saveMessages() {
        synchronized (mMessages) {
            for(int i=0; i<mMessages.size(); i++) {
                BLEBadgeMessage msg = mMessages.get(i);
                if (!msg.hasChanges())
                    continue;
                BluetoothGattCharacteristic ch = mMessageChars.get(i);
                msg.updateCharacteristic(ch);
                mQueue.add(GattQueueOperation.Write(ch));
            }
        }
    }

    // Connect to remote
    // Returns true if connected or connecting, false if not yet bonded
    public boolean connect() {
        Log.i(TAG, "Attempting to connect to remote badge.");
        if (connected()) {
            if (mBluetoothGatt == null) {
                mBluetoothGatt = mDevice.connectGatt(mContext, false, mGattCallback);
                mQueue = new GattQueue(mBluetoothGatt);
                mBluetoothGatt.discoverServices();
            }
            return true;
        }
        if (ensureBonded()) {
            Log.i(TAG, "Establishing GATT Connection.");
            mBluetoothGatt = mDevice.connectGatt(mContext, false, mGattCallback);
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
        Log.d(TAG, "Unregistering intent receiver.");
        try {
            mContext.unregisterReceiver(mBroadcastReceiver);
        } catch (java.lang.IllegalArgumentException ex) {
            // Just don't care, honestly.
        }

        if (mBluetoothGatt != null) {
            Log.i(TAG, "Disconnecting from GATT Server.");
            mBluetoothGatt.disconnect();
            mBluetoothGatt.close();
            mBluetoothGatt = null;
            mBadgeService = null;
        }
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
        Log.i(TAG, "Ensuring we are bonded now.");
        if (mDevice.getBondState() == BluetoothDevice.BOND_BONDED) {
            return true;
        }
        Log.i(TAG, "Not bonded, requesting a bonding.");
        boolean rv = mDevice.createBond();
        if (!rv) {
            Log.e(TAG, "Error requesting to create bond!");
        }
        return false;
    }

    // Updates all the characteristics
    private void updateCharacteristics() {
        Log.i(TAG, "Updating all characteristics.");
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
            mQueue.add(GattQueueOperation.Read(item));
        }
    }

    // Update the internal badge state
    private void updateState() {
        if (mBadgeService == null)
            return;

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

        // Update message index
        BluetoothGattCharacteristic index = mBadgeService.getCharacteristic(
                Constants.BadgeIndexUUID);
        if (index != null) {
            Integer intVal = index.getIntValue(BluetoothGattCharacteristic.FORMAT_SINT8, 0);
            mCurrentMessage = intVal.byteValue();
        } else {
            Log.w(TAG, "Could not find index characteristic in updateState.");
        }

        // Update messages
        UUID msgUUID = Constants.MessageUUID;
        List<BLEBadgeMessage> messages = new ArrayList<>();
        List<BluetoothGattCharacteristic> characteristics = new ArrayList<>();
        for (BluetoothGattCharacteristic item : mBadgeService.getCharacteristics()) {
            if (!item.getUuid().equals(msgUUID))
                continue;
            try {
                BLEBadgeMessage newMessage = BLEBadgeMessage.fromCharacteristic(item);
                messages.add(newMessage);
                characteristics.add(item);
            } catch(BLEBadgeException ex) {
                Log.e(TAG, "Unable to parse BLE Badge Message", ex);
            }
        }
        synchronized (this.mMessages) {
            mMessages.clear();
            mMessages.addAll(messages);
            mMessageChars.clear();
            mMessageChars.addAll(characteristics);
        }

        // Finally notify that state has changed
        notifyChanged();
    }

    // Represents a single badge message
    public static final class BLEBadgeMessage {

        private MessageMode mMode;
        private MessageSpeed mRate;
        private String mText;
        private boolean changed = false;

        private BLEBadgeMessage(MessageMode mode, MessageSpeed rate, String text) {
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
            MessageMode mode;
            try {
                mode = MessageMode.fromByte(readBuffer.get());
            } catch (BufferUnderflowException ex) {
                Log.e(TAG, "Buffer underflow!", ex);
                throw new BLEBadgeException("Characteristic too short!");
            }
            if (mode == null) {
                throw new BLEBadgeException("Unknown message mode!");
            }
            short rawRate;
            try {
                rawRate = readBuffer.getShort();
            } catch (BufferUnderflowException ex) {
                Log.e(TAG, "Buffer underflow!", ex);
                throw new BLEBadgeException("Characteristic too short!");
            }
            MessageSpeed rate = MessageSpeed.fromSpeed(rawRate);
            if (rate == null) {
                if (Constants.PermitUnknownRates) {
                    rate = MessageSpeed.fromNearest(rawRate);
                } else {
                    throw new BLEBadgeException("Unknown message rate!");
                }
            }
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

        public void updateCharacteristic(BluetoothGattCharacteristic characteristic) {
            characteristic.setValue(toBytes());
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
            if (text.equals(mText))
                return;
            mText = text;
            changed = true;
        }

        public MessageSpeed getSpeed() {
            return mRate;
        }

        public void setSpeed(MessageSpeed rate) {
            if (mRate == rate)
                return;
            mRate = rate;
            changed = true;
        }

        public MessageMode getMode() {
            return mMode;
        }

        public void setMode(MessageMode mode) {
            if (mMode == mode)
                return;
            mMode = mode;
            changed = true;
        }

        public boolean hasChanges() {
            return changed;
        }

        private byte[] toBytes() {
            final byte[] messageBytes;
            try {
                messageBytes = mText.getBytes("US-ASCII");
            } catch(UnsupportedEncodingException ex) {
                Log.e(TAG, "Unsupported encoding in toBytes!", ex);
                return null;
            }
            int length = MessageMode.SIZE + MessageSpeed.SIZE + messageBytes.length + 1;
            byte[] rawBuffer = new byte[length];
            ByteBuffer buffer = ByteBuffer.wrap(rawBuffer);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            buffer.put(mMode.encode());
            buffer.putShort(mRate.encode());
            buffer.put(messageBytes);
            buffer.put((byte)0); // ensure null termination on the firmware side
            return buffer.array();
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
                int reasonCode = intent.getIntExtra(
                        "android.bluetooth.device.extra.REASON", -1);
                BluetoothDevice affected = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                Log.d(TAG, "New bond state: " + BondStateName(newState) + " device: " + affected.getAddress());
                if (reasonCode != -1) {
                    Log.d(TAG, "Bond state reason: " + reasonCode);
                }
                if (newState == BluetoothDevice.BOND_BONDED && affected.equals(mDevice)) {
                    connect();
                }
            }
        }
    };

    private static final String BondStateName(int stateNum) {
        if (stateNum == BluetoothDevice.BOND_BONDED) {
            return "BOND_BONDED";
        } else if (stateNum == BluetoothDevice.BOND_BONDING) {
            return "BOND_BONDING";
        } else if (stateNum == BluetoothDevice.BOND_NONE) {
            return "BOND_NONE";
        }
        return "UNKNOWN";
    }

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
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                // Attempt to reconnect
                gatt.connect();
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
            // TODO: wtf?  This should not need reliable, as I never start reliable...
            mBluetoothGatt.executeReliableWrite();
            if (status == BluetoothGatt.GATT_SUCCESS) {
                int idx = mMessageChars.indexOf(characteristic);
                if (idx > -1) {
                    mMessages.get(idx).changed = false;
                }
            }
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
            super.onReliableWriteCompleted(gatt, status);
            Log.i(TAG, "Reliable write completed with status " + status);
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.e(TAG, "Failed to write changes to device.");
                notifyError("Failed to write changes to device.");
            }
            mQueue.executeNext();
        }
    };

    private enum GattOperation {
        READ, WRITE, RELIABLE_WRITE
    }

    private static final class GattQueueOperation {
        private final GattOperation op;
        private final BluetoothGattCharacteristic target;

        public GattQueueOperation(GattOperation op, BluetoothGattCharacteristic target) {
            this.op = op;
            this.target = target;
        }

        public static GattQueueOperation Write(BluetoothGattCharacteristic target) {
            return new GattQueueOperation(GattOperation.WRITE, target);
        }

        public static GattQueueOperation Read(BluetoothGattCharacteristic target) {
            return new GattQueueOperation(GattOperation.READ, target);
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
            boolean rv;
            if (op.op == GattOperation.READ) {
                Log.d(TAG, "Executing read.");
                rv = mGatt.readCharacteristic(op.target);
                if (!rv) {
                    Log.e(TAG, "Unable to read characteristic");
                }
                return rv;
            } else if (op.op == GattOperation.WRITE) {
                Log.d(TAG, "Executing write.");
                int writeType = op.target.getWriteType();
                String writeTypeString = "Unknown";
                if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) {
                    writeTypeString = "DEFAULT";
                } else if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) {
                    writeTypeString = "No Response";
                } else if (writeType == BluetoothGattCharacteristic.WRITE_TYPE_SIGNED) {
                    writeTypeString = "Signed";
                }
                Log.d(TAG, "Write type: " + writeTypeString);
                op.target.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
                rv = mGatt.writeCharacteristic(op.target);
                if (!rv) {
                    Log.e(TAG, "Unable to write Characteristic.");
                }
                return rv;
            } else if (op.op == GattOperation.RELIABLE_WRITE) {
                Log.d(TAG, "Executing reliable write.");
                if (!mGatt.beginReliableWrite()) {
                    Log.e(TAG, "Unable to begin reliable write.");
                }
                rv = mGatt.writeCharacteristic(op.target);
                if (!rv) {
                    Log.e(TAG, "Unable to write Characteristic.");
                }
                // Probably need to make this a separate operation
                if (!mGatt.executeReliableWrite()) {
                    Log.e(TAG, "Unable to execute reliable write.");
                }
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

package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Handler;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.Snackbar;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

public class BadgeSetupActivity extends AppCompatActivity
        implements NameChangeDialog.BadgeEditNameListener {
    private static final String TAG = "BadgeSetupActivity";

    private BluetoothDevice mDevice = null;
    private BLEBadge mBadge = null;

    private TextView mBadgeNameView = null;
    private TextView mBadgeAddressView = null;
    private Switch mBadgeDisplaySwitch = null;
    private SeekBar mBadgeBrightnessBar = null;
    private ProgressBar mLoadingSpinner = null;
    private Button mSaveButton = null;
    private Button mCancelButton = null;
    private Button mChangeNameButton = null;
    private RecyclerView mMessageView = null;
    private MessageListAdapter mMessageAdapter = null;

    private boolean mInRefresh = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_badge_setup);

        Intent intent = getIntent();
        if (!intent.hasExtra(Constants.BLEDevMessage)) {
            // Error due to lacking device
            Log.e(TAG, "Has no BLEDevMessage!");
            finish();
            return;
        }

        // Setup the badge
        mDevice = (BluetoothDevice)intent.getParcelableExtra(Constants.BLEDevMessage);
        mBadge = new BLEBadge(this, mDevice);
        mBadge.setUpdateNotifier(mUpdateNotifier);

        // Hold references to child views and set up actions
        findViews();
        mBadgeNameView.setText(mDevice.getName());
        mBadgeAddressView.setText(mDevice.getAddress());
        mBadgeBrightnessBar.setMax(Constants.MaxBrightness);
        mBadgeDisplaySwitch.setOnCheckedChangeListener(mDisplayChangeListener);
        mCancelButton.setOnClickListener(mCancelListener);
        mSaveButton.setOnClickListener(mSaveListener);
        mBadgeBrightnessBar.setOnSeekBarChangeListener(mBrightnessListener);
        mChangeNameButton.setOnClickListener(mChangeNameListener);

        // Setup the recycler view
        LinearLayoutManager recyclerManager = new LinearLayoutManager(this);
        mMessageView.setHasFixedSize(true);
        mMessageView.setLayoutManager(recyclerManager);
        mMessageAdapter = new MessageListAdapter(mMessageChanger);
        mMessageView.setAdapter(mMessageAdapter);

        // Disable buttons until we're connected
        setInterfaceEnabled(false);
    }

    private void findViews() {
        mBadgeNameView = findViewById(R.id.badge_name);
        mBadgeAddressView = findViewById(R.id.badge_address);
        mBadgeDisplaySwitch = findViewById(R.id.display_switch);
        mBadgeBrightnessBar = findViewById(R.id.brightness_seekbar);
        mLoadingSpinner = findViewById(R.id.badge_load_progress);
        mCancelButton = findViewById(R.id.cancel_btn);
        mSaveButton = findViewById(R.id.save_btn);
        mMessageView = findViewById(R.id.message_recycler_view);
        mChangeNameButton = findViewById(R.id.name_change_btn);
    }

    @Override
    protected void onStart() {
        super.onStart();
        mBadge.connect();
    }

    @Override
    protected void onStop() {
        super.onStop();
        mBadge.close();
    }

    /*
     * Enable/disable controls in bulk.
     */
    private void setInterfaceEnabled(boolean enabled) {
        mChangeNameButton.setEnabled(enabled);
        mBadgeDisplaySwitch.setEnabled(enabled);
        mBadgeBrightnessBar.setEnabled(enabled);
        mCancelButton.setEnabled(enabled);
        mSaveButton.setEnabled(enabled);
    }

    private void displayErrorSnackbar(final String message) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                CoordinatorLayout coordinatorLayout = findViewById(R.id.home_coordinator);
                Snackbar.make(coordinatorLayout, message, Snackbar.LENGTH_LONG);
            }
        });
    }

    private final BLEBadge.BLEBadgeUpdateNotifier mUpdateNotifier = new BLEBadge.BLEBadgeUpdateNotifier() {
        @Override
        void onChanged(final BLEBadge badge) {
            // Update view
            Log.i(TAG, "Notified of BLE Badge Change.");
            BadgeSetupActivity.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        mInRefresh = true;

                        // Basic information
                        mBadgeNameView.setText(mDevice.getName());
                        mBadgeDisplaySwitch.setChecked(badge.getDisplayEnabled());
                        mBadgeBrightnessBar.setProgress(badge.getBrightness());

                        // Set the messages.
                        mMessageAdapter.setMessages(badge.getMessages());

                        // Hide the spinner
                        mLoadingSpinner.setVisibility(View.GONE);

                        // Show the selected message
                        mMessageAdapter.setCheckedPosition(mBadge.getCurrentMessage());

                        // Ensure controls are enabled
                        setInterfaceEnabled(true);
                    } finally {
                        mInRefresh = false;
                    }
                }
            });
        }

        @Override
        void onError(BLEBadge badge, String error) {
            displayErrorSnackbar(error);
        }
    };

    private final CompoundButton.OnCheckedChangeListener mDisplayChangeListener = new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            if (mInRefresh)
                return;
            mBadge.setDisplayEnabled(isChecked);
        }
    };

    private final SeekBar.OnSeekBarChangeListener mBrightnessListener = new SeekBar.OnSeekBarChangeListener() {
        private static final int delayMs = 100;  // Don't immediately update when being dragged
        private boolean updateInFlight = false;
        private int newValue;
        private final Handler handler = new Handler();

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            if (!fromUser)
                return;
            newValue = progress;
            synchronized (mBrightnessListener) {
                if (updateInFlight)
                    return;
                updateInFlight = true;
            }
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    try {
                        mBadge.setBrightness((byte) newValue);
                    } catch (BLEBadge.BLEBadgeException ex) {
                        displayErrorSnackbar(ex.toString());
                    } finally {
                        synchronized (mBrightnessListener) {
                            updateInFlight = false;
                        }
                    }
                }
            }, delayMs);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) { }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) { }
    };

    private View.OnClickListener mCancelListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            // Go back to previous
            Log.i(TAG, "Cancel clicked.");
            finish();
        }
    };

    private View.OnClickListener mSaveListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mBadge.saveMessages();
            Log.i(TAG, "Save clicked.");
        }
    };

    private MessageListAdapter.IActiveMessageChanger mMessageChanger = new MessageListAdapter.IActiveMessageChanger() {
        @Override
        public void onActiveMessageChanged(int pos) {
            try {
                mBadge.setCurrentMessage((byte) pos);
            } catch (BLEBadge.BLEBadgeException ex) {
                Log.e(TAG, "Error changing message.", ex);
                displayErrorSnackbar(ex.toString());
            }
        }
    };

    private View.OnClickListener mChangeNameListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            DialogFragment dialog = new NameChangeDialog();
            dialog.show(getSupportFragmentManager(), "NameChangeDialog");
        }
    };

    public String getCurrentName() {
        if (mBadge != null)
            return mBadge.getName();
        return "";
    }

    public void setNewName(String name) {
        if (mBadge == null) {
            Log.e(TAG, "Changing name with no associated badge!");
            return;
        }
        mBadge.setName(name);
    }
}

package com.attackercommunity.acdcbadge;

import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Handler;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.Snackbar;
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

public class BadgeSetupActivity extends AppCompatActivity {
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

        // Setup the recycler view
        LinearLayoutManager recyclerManager = new LinearLayoutManager(this);
        mMessageView.setHasFixedSize(true);
        mMessageView.setLayoutManager(recyclerManager);
        mMessageAdapter = new MessageListAdapter();
        mMessageView.setAdapter(mMessageAdapter);
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
        private static final int delayMs = 50;  // Don't immediately update when being dragged
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
                handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (mBrightnessListener) {
                            updateInFlight = false;
                        }
                        try {
                            mBadge.setBrightness((byte) newValue);
                        } catch (BLEBadge.BLEBadgeException ex) {
                            displayErrorSnackbar(ex.toString());
                        }
                    }
                }, delayMs);

            }
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
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
            // TODO: implement this!
            Log.i(TAG, "Save clicked.");
        }
    };
}

package com.attackercommunity.acdcbadge;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import java.util.ArrayList;

public class HomeActivity extends AppCompatActivity {
    public static final String TAG = "HomeActivity";
    private static final int REQUEST_ENABLE_BT = 1;
    private static final int REQUEST_PERMISSIONS = 2;

    private BadgeListAdapter mBadgeListAdapter;
    private View mRootView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);
        mRootView = findViewById(R.id.home_coordinator);
        if (mRootView == null) {
            Log.e(TAG, "rootView is null!");
        }
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        // TODO: Make this refresh
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mBadgeListAdapter.refreshView(view.getContext());
            }
        });

        // Setup the recycler view
        RecyclerView recyclerView = (RecyclerView) findViewById(R.id.badge_recycler);
        recyclerView.setHasFixedSize(true);
        LinearLayoutManager layoutManager = new LinearLayoutManager(this);
        recyclerView.setLayoutManager(layoutManager);
        mBadgeListAdapter = new BadgeListAdapter(mRootView, this);
        recyclerView.setAdapter(mBadgeListAdapter);
        DividerItemDecoration decoration = new DividerItemDecoration(
                recyclerView.getContext(), layoutManager.getOrientation());
        recyclerView.addItemDecoration(decoration);

        // Prompt for bluetooth if needed
        if(checkPermissions()) {
            final BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            BluetoothAdapter adapter = bluetoothManager.getAdapter();
            if (adapter == null || !adapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            } else {
                mBadgeListAdapter.refreshView(this);
            }
        } else {
            Log.i(TAG, "Waiting on permissions.");
            Snackbar.make(mRootView, "Waiting on permissions.", Snackbar.LENGTH_LONG);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_home, menu);
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_ENABLE_BT && resultCode == RESULT_OK) {
            mBadgeListAdapter.refreshView(this);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] perms, int[] results) {
        boolean gotAll = results.length > 0;
        for (int res : results) {
            gotAll = gotAll && (res == PackageManager.PERMISSION_GRANTED);
        }
        if (!gotAll) {
            // Toast complaining about permissions
            Snackbar.make(findViewById(R.id.badge_recycler),
                    "Don't have the necessary permissions.",
                    Snackbar.LENGTH_INDEFINITE);
        } else {
            // Restart activity with new permissions
            recreate();
        }
    }

    private boolean checkPermissions() {
        String[] needed = {
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_COARSE_LOCATION,
        };
        ArrayList<String> requested = new ArrayList<>();
        for (String perm : needed) {
            if (ContextCompat.checkSelfPermission(this, perm) != PackageManager.PERMISSION_GRANTED) {
                requested.add(perm);
            }
        }
        if (requested.size() == 0) {
            return true;
        }
        String[] toRequest = requested.toArray(new String[requested.size()]);
        ActivityCompat.requestPermissions(this, toRequest, REQUEST_PERMISSIONS);
        return false;
    }
}

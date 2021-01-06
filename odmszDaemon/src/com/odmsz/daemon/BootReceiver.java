package com.odmsz.daemon;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;


public class BootReceiver extends BroadcastReceiver {
    private static final String TAG = "odmszdaemon";

    @Override
    public void onReceive(final Context context, Intent intent) {
        String action = intent.getAction();
        Log.d(TAG, "action="+action);
        Intent service = new Intent(context, MainService.class);

        context.startService(service);
    }

    
}
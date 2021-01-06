package com.odmsz.daemon;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import android.view.KeyEvent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

public class MainActivity extends Activity {
    private final String TAG = "odmszdaemon";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.w(TAG, "onKeyDown keyCode="+keyCode+" event="+event.getAction());
        if ((keyCode == KeyEvent.KEYCODE_POWER)) {
            return false;
        }else {
            return super.onKeyDown(keyCode, event);
        }
    }
}

package com.odmsz.daemon;

import android.app.Service;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;

import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import com.odmsz.daemon.ShellUtils;
import com.odmsz.daemon.ShellUtils.CommandResult;


import android.os.ServiceManager;
import android.os.SystemProperties;

public class MainService extends Service{
    private static final String TAG = "odmszdaemon";
    private static IntentFilter filter = new IntentFilter();
    private static final String SHELL_COMMAND_REQUEST_INTENT="com.odmsz.daemon.shell.request";
    private static final String SHELL_COMMAND_RESPOND_INTENT="com.odmsz.daemon.shell.respond";
    private static Context mContext = null;

    @Override
    public void onCreate() {
        Log.i(TAG,"onCreate");
        super.onCreate();
        mContext = getApplicationContext();
        filter.addAction(SHELL_COMMAND_REQUEST_INTENT);

        mContext.registerReceiver(mReceiver, filter);
    }

    public static String runSystemCmd(String cmd)
    {
        //Log.d(TAG,"runSystemCmd : "+cmd);
        CommandResult ret = ShellUtils.execCommand(cmd,false,true,TAG);
        if (ret.successMsg!=null)
            return ret.successMsg;
        else if (ret.errorMsg!=null)
            return ret.errorMsg;
        else
            return "failed";
    } 

    private static void reboot()
    {
        Intent intent=new Intent(Intent.ACTION_REBOOT);
        intent.putExtra("nowait", 1);
        intent.putExtra("interval", 1);
        intent.putExtra("window", 0);
        mContext.sendBroadcast(intent);
    }

    private static BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context c, Intent intent) {

            String cmd = "psh"; 
            String result = "null";
            String respond = "null";
            //Log.i(TAG,"onReceive intent "+intent.getAction());

            if (intent.getAction().equals(SHELL_COMMAND_REQUEST_INTENT))
            {
                String bt_addr = intent.getStringExtra("bt");
                if (bt_addr!=null)
                {
                    Log.d(TAG,"write bt_addr "+bt_addr);
                    cmd = "psh nvtool write bt "+ bt_addr;
                    result = runSystemCmd(cmd);
                    respond = "bt:"+result+";";
                }
                String mac = intent.getStringExtra("mac");
                if (mac!=null)
                {
                    Log.d(TAG,"write mac "+mac);
                    cmd = "psh macplugin "+ mac;
                    result = runSystemCmd(cmd);
                    respond += "mac:"+result+";";
                }
                String imei = intent.getStringExtra("imei");
                if (imei!=null)
                {
                    Log.d(TAG,"write imei "+imei);
                    cmd = "psh nvtool write imei "+ imei;
                    result = runSystemCmd(cmd);
                    respond += "imei:"+result+";";
                }
                String sn = intent.getStringExtra("sn");
                if (sn!=null)
                {
                    Log.d(TAG,"write sn "+sn);
                    cmd = "psh snwriter "+ sn;
                    result = runSystemCmd(cmd);
                    respond += "sn:"+result+";";
                }

                boolean reboot = intent.getBooleanExtra("reboot",false);
                if (reboot)
                {
                    Log.d(TAG,"reboot now ");
                    reboot();
                }else
                    notifyShellRunResult(respond);
            }
        }
    };

    private static void notifyShellRunResult(String repsond)
     {
         Log.d(TAG,SHELL_COMMAND_RESPOND_INTENT+" "+repsond);
         Intent intent = new Intent(SHELL_COMMAND_RESPOND_INTENT);
         intent.putExtra("repsond",repsond);
         mContext.sendBroadcast(intent);
     }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG,"onStartCommand");
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG,"onBind");
        return null;
    }


    @Override
    public void onDestroy() {
        Log.i(TAG,"onDestroy");
        super.onDestroy();
    }

}

package com.odmsz.daemon;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.view.View;
import android.util.Log;
import android.widget.Toast;
import android.content.Intent;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.lang.StringBuffer;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import com.odmsz.daemon.ShellUtils;
import com.odmsz.daemon.ShellUtils.CommandResult;
import java.nio.charset.Charset;


public class DeviceInfo extends Activity {
    static final String TAG="odmszdaemon";
    static final String SERIAL_KEY="ro.boot.serialno";
    Map<String,String> mapProp = new HashMap<String,String>();
    EditText imei_EditText;
    EditText sn_EditText;
    EditText mac_EditText;
    EditText bt_EditText;

    Button button_write;
    Button button_read;
    Button button_reboot;

    buttonOnClick mButtonListener = new buttonOnClick();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_info_activity);
        imei_EditText = (EditText) findViewById(R.id.editText_imei);
        sn_EditText =  (EditText) findViewById(R.id.editText_sn);
        mac_EditText =  (EditText) findViewById(R.id.editText_mac);
        bt_EditText =  (EditText) findViewById(R.id.editText_bt);
        button_write = (Button) findViewById(R.id.button_write);
        button_write.setOnClickListener(mButtonListener);
        button_read = (Button) findViewById(R.id.button_read);
        button_read.setOnClickListener(mButtonListener);
        button_reboot = (Button) findViewById(R.id.button_reboot);
        button_reboot.setOnClickListener(mButtonListener);
    }
    private void showToast(String str)
    {
        Toast toast = Toast.makeText(getApplicationContext(),
                str, Toast.LENGTH_LONG);
        toast.setGravity(Gravity.CENTER, 0, 0);
        toast.show();
    }

    private String stringToMacString(String mac)
    {
        byte [] bytes = HexStringToBytes(mac);
        
        return String.format("%02x:%02x:%02x:%02x:%02x:%02x",
            bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5]);

    }

    public static String bytesToHexString(byte[] bytes) {
        if (bytes == null) return null;
        StringBuilder ret = new StringBuilder(2*bytes.length);
        for (int i = 0 ; i < bytes.length ; i++) {
            int b;
            b = 0x0f & (bytes[i] >> 4);
            ret.append("0123456789abcdef".charAt(b));
            b = 0x0f & bytes[i];
            ret.append("0123456789abcdef".charAt(b));
        }
        return ret.toString();
    }


    public static byte[] HexStringToBytes(String str) {
        if (str == null) return null;
        byte[] mbytes = new byte[str.length()/2];
        for (int i = 0 ; i <str.length() ; i=i+2) {
            mbytes[i/2]=(byte)(((("0123456789abcdef".indexOf(str.charAt(i)))<<4)&0xf0)|
                            (("0123456789abcdef".indexOf(str.charAt(i+1)))&0x0f));
        }
        return mbytes;
    }

      

    public String runSystemCmd(String cmd)
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
    private int readInfo(){
        try{
            String cmd = "psh nvtool read bt";
            String ret = runSystemCmd(cmd);
            BufferedReader br = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(ret.getBytes(Charset.forName("utf8"))), Charset.forName("utf8")));  
            parseStringsIntoMap(br, "=");
            br.close();
            String info = mapProp.get("BT");
            
            if (info!=null)
                bt_EditText.setText(info);

            cmd = "psh nvtool read imei";
            ret = runSystemCmd(cmd);
            br = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(ret.getBytes(Charset.forName("utf8"))), Charset.forName("utf8")));  
            parseStringsIntoMap(br, "=");
            br.close();
            info = mapProp.get("IMEI");
            if (info!=null)
                imei_EditText.setText(info);

            cmd = "psh cat /persist/wlan_mac.bin";
            ret = runSystemCmd(cmd);
            br = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(ret.getBytes(Charset.forName("utf8"))), Charset.forName("utf8")));  
            parseStringsIntoMap(br, "=");
            br.close();
            info = mapProp.get("Intf0MacAddress");
            if (info!=null)
                mac_EditText.setText(info);

            cmd = "psh cat /persist/odmsz.prop";
            ret = runSystemCmd(cmd);
            br = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(ret.getBytes(Charset.forName("utf8"))), Charset.forName("utf8")));  
            parseStringsIntoMap(br, "=");
            br.close();
            info = mapProp.get(SERIAL_KEY);
            if (info!=null)
                sn_EditText.setText(info);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return 0;
    }
    private int writeInfo() {

        String cmd = "psh";
        String info;
        if (sn_EditText.getText()!=null)
        {
            cmd = "snwriter "+sn_EditText.getText().toString();
            runSystemCmd(cmd);
        }

        if ((imei_EditText.getText())!=null)
        {
            info = imei_EditText.getText().toString();
            if (info.length()==15||info.length()==14)
            {
                cmd = "psh nvtool write imei "+info;
                runSystemCmd(cmd);
            }
            else {
                showToast(getApplicationContext().getString(R.string.imei_lenght_wrong));
            }
            
        }
        if ((mac_EditText.getText())!=null) {
            info = mac_EditText.getText().toString();
            if (info.length()==12) {
                cmd = "psh macplugin " + info;
                runSystemCmd(cmd);
            }
            else {
                showToast(getApplicationContext().getString(R.string.mac_lenght_wrong));
            }
        }

        if ((bt_EditText.getText())!=null){
            info = bt_EditText.getText().toString();
            
            if (info.length()==12) {
                cmd = "psh nvtool write bt " + info;
                runSystemCmd(cmd);
            }
            else {
                showToast(getApplicationContext().getString(R.string.bt_lenght_wrong));
            }
        }
        return 0;
    }

    private void reboot()
    {
        Intent intent=new Intent(Intent.ACTION_REBOOT);
        intent.putExtra("nowait", 1);
        intent.putExtra("interval", 1);
        intent.putExtra("window", 0);
        sendBroadcast(intent);
    }
    
    private class buttonOnClick implements OnClickListener {
        @Override
        public void onClick(View view) {

            switch (view.getId()) {
                case R.id.button_write:
                    writeInfo();
                    break;
                case R.id.button_read:
                    readInfo();
                    break;
                case R.id.button_reboot:
                    reboot();
                    break;
            }

        }
    }

    public void parseStringsIntoMap(BufferedReader bufferreader, String split)
    {
        try{
            String readStr;
            while ((readStr = bufferreader.readLine()) != null) {
                if(readStr.split(split).length==2)
                {
                    mapProp.put(readStr.split(split)[0],readStr.split(split)[1]);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public void readFile(String filename)
    {
        String content = null;
        File file = new File(filename); //for ex foo.txt
        try {
            FileReader filereader = new FileReader(file);
            BufferedReader bufferreader = new BufferedReader(filereader);

            parseStringsIntoMap(bufferreader,"=");
            bufferreader.close();
            filereader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void writeFile(String filename){
        FileWriter fw;
        try {
            fw = new FileWriter(filename);
            BufferedWriter bw=new BufferedWriter(fw);
            Set<String> key = mapProp.keySet();
            for (Iterator it = key.iterator(); it.hasNext();) {
                String s = (String) it.next();
                bw.write(s+"="+mapProp.get(s));
                Log.d(TAG,"writeFile="+s+"="+mapProp.get(s));
                bw.newLine();
            }
            bw.close();
            fw.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

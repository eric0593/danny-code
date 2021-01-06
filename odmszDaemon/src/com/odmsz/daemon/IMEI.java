/**
 * Created by Administrator on 2014/6/29.
 */
package com.odmsz.daemon;

public class IMEI {
    static byte CalcIMEISP(String imeiStr)
    {
        int SP;
        byte oH = 0, oL = 0;

        SP  = imeiStr.charAt(1) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(3) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(5) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(7) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(9) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(11) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = imeiStr.charAt(13) - '0';
        SP  = 2*SP;
        oH += SP/10;
        oL += SP%10;

        SP  = 0;
        SP += imeiStr.charAt(0) - '0';
        SP += imeiStr.charAt(2) - '0';
        SP += imeiStr.charAt(4) - '0';
        SP += imeiStr.charAt(6) - '0';
        SP += imeiStr.charAt(8) - '0';
        SP += imeiStr.charAt(10) - '0';
        SP += imeiStr.charAt(12) - '0';
        SP += oH + oL;

        SP = SP%10;
        if(SP > 0)
        {
            SP = 10 - SP;
        }
        return (byte)SP;
    }

    static void String2Nv(String imeiStr, byte[] pImei, boolean m_bIMEI15Enable)
    {
        int SP;
        if(!m_bIMEI15Enable)
        {
            SP = CalcIMEISP(imeiStr);
        }
        else
        {
            SP = imeiStr.charAt(14) - '0';
        }

        pImei[0] = 0x08;
        pImei[1] =(byte)(0x0A | ((imeiStr.charAt(0) - '0')<<4)); // IMEI固定第一位是0xA
        pImei[2] = (byte)(((imeiStr.charAt(1) - '0')) | ((imeiStr.charAt(2) - '0')<<4));
        pImei[3] = (byte)(((imeiStr.charAt(3) - '0')) | ((imeiStr.charAt(4) - '0')<<4));
        pImei[4] = (byte)(((imeiStr.charAt(5) - '0')) | ((imeiStr.charAt(6) - '0')<<4));
        pImei[5] = (byte)(((imeiStr.charAt(7) - '0')) | ((imeiStr.charAt(8) - '0')<<4));
        pImei[6] = (byte)(((imeiStr.charAt(9) - '0')) | ((imeiStr.charAt(10) - '0')<<4));
        pImei[7] = (byte)(((imeiStr.charAt(11) - '0')) | ((imeiStr.charAt(12) - '0')<<4));
        pImei[8] = (byte)(((imeiStr.charAt(13) - '0')) | (SP<<4));

    }

     String Nv2String(byte[] pImei)
    {
        int ch;
        byte[] imei=new byte[15];

        ch = ((pImei[1]&0xF0)>>4)+'0';
        imei[0]=(byte)ch;
        ch = ((pImei[2]&0x0F))+'0';
        imei[1]=(byte)ch;
        ch = ((pImei[2]&0xF0)>>4)+'0';
        imei[2]=(byte)ch;
        ch = ((pImei[3]&0x0F))+'0';
        imei[3]=(byte)ch;
        ch = ((pImei[3]&0xF0)>>4)+'0';
        imei[4]=(byte)ch;
        ch = ((pImei[4]&0x0F))+'0';
        imei[5]=(byte)ch;
        ch = ((pImei[4]&0xF0)>>4)+'0';
        imei[6]=(byte)ch;
        ch = ((pImei[5]&0x0F))+'0';
        imei[7]=(byte)ch;
        ch = ((pImei[5]&0xF0)>>4)+'0';
        imei[8]=(byte)ch;
        ch = ((pImei[6]&0x0F))+'0';
        imei[9]=(byte)ch;
        ch = ((pImei[6]&0xF0)>>4)+'0';
        imei[10]=(byte)ch;
        ch = ((pImei[7]&0x0F))+'0';
        imei[11]=(byte)ch;
        ch = ((pImei[7]&0xF0)>>4)+'0';
        imei[12]=(byte)ch;
        ch = ((pImei[8]&0x0F))+'0';
        imei[13]=(byte)ch;
        ch = ((pImei[8]&0xF0)>>4)+'0';
        imei[14]=(byte)ch;

        String  imeiStr=new String(imei);
        return imeiStr;
    }
}

/* system/bin/netcfg/netcfg.c
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#define NDEBUG 0
#include <stdio.h>
#include <cutils/properties.h>
#define LOG_TAG "ideatool"
#include <cutils/log.h>
extern int talkto6270 (void);

void reboot4gmodem()
{
    int ret = 0;
    ret = system("echo related > /sys/bus/msm_subsys/devices/subsys2/restart_level");
    ALOGE ("###############reboot4gmodem cmd1 = %d###############",ret);
    ret = system("echo restart > /sys/kernel/debug/msm_subsys/modem");
    ALOGE ("###############reboot4gmodem cmd2 = %d###############",ret);    
}

void reboot3gModemAll(void)
{
    int ret=0;
	ALOGE ("###############reboot3gModemAll function 0 ###############");
    property_set("ctl.stop","pppd_gprs");
    usleep(1000);
    property_set("net.wcdma.ppp-exit","");
    property_set("net.wcdma.errorno","");
    property_set("ctl.stop","mux");
    usleep(1000);	
    property_set("ctl.stop","3gril-daemon");
    usleep(1000);	
    ret = system("echo 1 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd1 = %d###############",ret);
    usleep(100*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd2 = %d###############",ret);
    usleep(20*1000*1000);
    property_set("ctl.start","3gril-daemon");
    
}

void reboot3gModemAll1(void)
{
    int ret=0;
	ALOGE ("###############reboot3gModemAll function 1 ###############");
    property_set("ctl.stop","pppd_gprs");
    usleep(1000);
    property_set("net.wcdma.ppp-exit","");
    property_set("net.wcdma.errorno","");
    property_set("ctl.stop","mux");
    usleep(1000);
    property_set("ctl.stop","3gril-daemon");
    usleep(1000);
    ret = system("echo 1 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd1 = %d###############",ret);
    usleep(200*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd2 = %d###############",ret);    
    usleep(20*1000*1000);
    property_set("ctl.start","3gril-daemon");
    
}

void reboot3gModemAll2(void)
{
    int ret=0;
	ALOGE ("###############reboot3gModemAll function 2 ###############");
    property_set("ctl.stop","pppd_gprs");
    usleep(1000);
    property_set("net.wcdma.ppp-exit","");
    property_set("net.wcdma.errorno","");
    property_set("ctl.stop","mux");
    usleep(1000);
    property_set("ctl.stop","3gril-daemon");
    usleep(1000);
    ret = system("echo 1 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd1 = %d###############",ret);
    usleep(200*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModemAll cmd2 = %d###############",ret);
    poweron3gModem();
    usleep(20*1000*1000);
    property_set("ctl.start","3gril-daemon");
    
}



void reboot3gModem(void)
{
    int ret=0;
    ret = system("echo 1 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModem cmd1 = %d###############",ret);
    usleep(200*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio921/value");
    ALOGE ("###############reboot3gModem cmd2 = %d###############",ret);
}

void configReboot3gModem(void)
{
    int ret = 0;
    ret = system("echo low > /sys/class/gpio/gpio921/direction");
    ALOGE ("###############configReboot3gModem cmd1 = %d###############",ret);    
}

void poweroff3gModem(void)
{
    int ret=0;
    configPoweroff3gModem();
    talkto6270();
    ret = system("echo 1 > /sys/class/gpio/gpio943/value");
    ALOGE ("###############poweroff3gModem cmd1 = %d###############",ret);
    usleep(1000*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio943/value");
    ALOGE ("###############poweroff3gModem cmd2 = %d###############",ret);    
    property_set("ril.umtsphone.shutdown.ready", "1");
}


void poweron3gModem(void)
{
    int ret=0;
    usleep(200*1000);
    ret = system("echo 1 > /sys/class/gpio/gpio943/value");
    ALOGE ("###############poweron3gModem cmd1 = %d###############",ret);
    usleep(500*1000);
    ret = system("echo 0 > /sys/class/gpio/gpio943/value");
    ALOGE ("###############poweron3gModem cmd2 = %d###############",ret);    
}


void configPoweroff3gModem(void)
{
    int ret = 0;
    ret = system("echo 943 > /sys/class/gpio/export");
    ALOGE ("###############configPoweroff3gModem cmd1 = %d###############",ret);
    ret = system("echo low > /sys/class/gpio/gpio943/direction");
    ALOGE ("###############configPoweroff3gModem cmd2 = %d###############",ret);
}

void runCmd(void)
{
    int ret = 0;
    char cmd1[256] = {0};
    char cmd2[256] = {0};
    char cmd3[256] = {0};
    property_get("ideatool.cmd1", cmd1, "none");
    if (!strcmp(cmd1,"none"))
        return;
    *(cmd1+strlen(cmd1))=' ';
    property_get("ideatool.cmd2", cmd2, "none");
    if (strcmp(cmd1,"none"))
    {
        strcat(cmd1,cmd2);
        *(cmd1+strlen(cmd1))=' ';
    }
    
    property_get("ideatool.cmd3", cmd3, "none");
    if (strcmp(cmd3,"none"))
    {
        strcat(cmd1,cmd3);
        *(cmd1+strlen(cmd1))=' ';
    }

    ret = system(cmd1);
    ALOGD ("###############runCmd cmd = %s ret=%d###############",cmd1,ret);
}


int main(int argc, char **argv)
{
    char func[PROPERTY_VALUE_MAX] = {0};
    property_get("ideatool.poweroff", func, "false");
    if (!strcmp(func,"true"))
    {        
        ALOGE("ideatool wait for ideatool_poweroff shutdown\n");
        return 0;
    }
    
    property_get("ideatool.func", func, "config");

    ALOGE("###############ideatool func = %s########################\n",func);
    if (!strcmp(func,"config"))
    {        
        configReboot3gModem();
        configPoweroff3gModem();        
    }  
    else if (!strcmp(func,"rb4g"))
    {
        reboot4gmodem();
    }
    //else if(!strcmp(func,"crb3g"))
    //{
    //    configReboot3gModem();
    //}  
    else if(!strcmp(func,"rb3ga"))
    {
        reboot3gModemAll();
    } 
    else if(!strcmp(func,"rb3ga1"))
    {
        reboot3gModemAll1();
    }  
    else if(!strcmp(func,"rb3ga2"))
    {
        reboot3gModemAll2();
    }      
    else if(!strcmp(func,"rb3g"))
    {
        reboot3gModem();
    }
    else if(!strcmp(func,"pon3g"))
    {
        poweron3gModem();
    }    
    else if(!strcmp(func,"poff3g"))
    {
        poweroff3gModem();
    }    
    else if(!strcmp(func,"cmd"))
    {
        runCmd();
    }  

    return 0;
}


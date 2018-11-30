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
#define NDEBUG 1

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/stat.h>
#include <time.h>



#include <string.h>
#define LOG_TAG "macplugin"
#define WLAN_MAC_FILE "/persist/wlan_mac.bin"



static void array2str(uint8_t *array,char *str) {
    int i;
    char c;
    for (i = 0; i < 6; i++) {
        c = (array[i] >> 4) & 0x0f; //high 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'a'-32;
        }

        *str ++ = c;
        c = array[i] & 0x0f; //low 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'a'-32;
        }
        *str ++ = c;
    }
    *str = 0;
}

     /* data format:
        * Intf0MacAddress=00AA00BB00CC
        * Intf1MacAddress=00AA00BB00CD
        * END
        */
int main(int argc, char **argv)
{
    char serial[PROPERTY_VALUE_MAX] = {0};
    char buff[56] = {0};
    int fd = 0;
    size_t sz = 0;
    uint8_t randomMac[6];
    uint8_t strMac[20];
    srand(time(NULL));    
    randomMac[0] = 0x02;
    randomMac[1] = 0xaa;
    randomMac[2] = (rand() & 0x0FF00000) >> 20;
    randomMac[3] = (rand() & 0x0FF00000) >> 20;
    randomMac[4] = (rand() & 0x0FF00000) >> 20;
    randomMac[5] = (rand() & 0x0FF00000) >> 20;
    //property_get("ro.boot.serialno", serial, "");	  
    //ALOGD("->get serail no = %s\n",serial);
    
    if((access(WLAN_MAC_FILE,F_OK))!=-1)   
    {   
        ALOGD("->WLAN_MAC_FILE exist\n");
        return 0;
    }  
    
    fd = open(WLAN_MAC_FILE,O_CREAT|O_RDWR,0666);
    if (fd<0)
    {
    	ALOGD("->WLAN_MAC_FILE create failed fd=%d\n",fd);
    	return -1;
    }
    
    strcpy(buff,"Intf0MacAddress=");
    array2str(randomMac, strMac);
    strcat(buff+strlen(buff),strMac);
    buff[strlen(buff)]=0x0A;
    sz = write(fd,buff,strlen(buff));
    ALOGD("->Intf0MacAddress=%s strlen=%d sz=%d\n",buff,strlen(buff),sz);
    
    memset(buff,0x00,sizeof(buff));
    strcpy(buff,"Intf1MacAddress=");
    randomMac[1]=0xcc;
    array2str(randomMac, strMac);
    strcat(buff+strlen(buff),strMac);    
    buff[strlen(buff)]=0x0A;    
    sz = write(fd,buff,strlen(buff));    
    ALOGD("->Intf1MacAddress=%s strlen=%d sz=%d\n",buff,strlen(buff),sz);

    memset(buff,0x00,sizeof(buff));
    strcpy(buff,"END");
    buff[strlen(buff)]=0x0A;    
    sz = write(fd,buff,strlen(buff)); 
    
    close(fd);
    return 0;
}


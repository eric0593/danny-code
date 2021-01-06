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
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "macplugin"
#endif
#define WLAN_MAC_FILE "/persist/wlan_mac.bin"

void hex_to_bcd(unsigned char * hex, char * bcd, int len)
{
    int i = 0;
    unsigned char low = 0, high = 0;

    for (i=0;i<len;i++)
    {
        low = (hex[i]&0x0f);
        high = ((hex[i]&0xf0)>>4);
        bcd[2*i] = high >=10? (high-10+'A'):(high+'0');
        bcd[2*i+1] = low >=10? (low-10+'A'):(low+'0');
    }
}

int bcd_to_hex(char * bcd, unsigned char * hex, int len)
{
    int i = 0;
    char low = 0, high = 0;
    for (i=0;i<len;i++)
    {
        high = bcd[2*i];
        low = bcd[2*i+1];
        if (low >= 'a' && low <= 'f')
            low = low - 'a' + 10;
        else if (low >= 'A' && low <= 'F')
            low = low - 'A'+ 10;
        else if (low >= '0' && low <='9' )
            low = low - '0';
        else 
            return -1;

        if (high >= 'a' && high <= 'f')
            high = high - 'a'+ 10;
        else if (high >= 'A' && high <= 'F')
            high = high - 'A'+ 10;
        else if (high >= '0' && high <='9' )
            high = high - '0';
        else 
            return -1;

        hex[i] = (high<<4)+(low);
    }
    return 0;
}
/*
static void array2str(uint8_t *array,char *str) {
    int i;
    char c;
    for (i = 0; i < 6; i++) {
        c = (array[i] >> 4) & 0x0f; //high 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'A';
        }

        *str ++ = c;
        c = array[i] & 0x0f; //low 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'A';
        }
        *str ++ = c;
    }
    *str = 0;
}
*/
     /* data format:
        * Intf0MacAddress=00AA00BB00CC
        * Intf1MacAddress=00AA00BB00CD
        * END
        */

void write_mac_to_file(char *mac0, char* mac1, bool force)
{
      int fd = 0;
      char buff[56] = {0};
      size_t sz = 0;
        if((access(WLAN_MAC_FILE,F_OK))!=-1&&(force!=true))
    {
        ALOGD("->WLAN_MAC_FILE exist\n");
        return;
    }

    fd = open(WLAN_MAC_FILE,O_CREAT|O_RDWR,0666);
    if (fd<0)
    {
        ALOGD("->WLAN_MAC_FILE create failed fd=%d\n",fd);
        return;
    }

    strcpy(buff,"Intf0MacAddress=");
    //array2str(randomMac, strMac);
    strcat(buff+strlen(buff),mac0);
    buff[strlen(buff)]=0x0A;
    sz = write(fd,buff,strlen(buff));
    ALOGD("Intf0MacAddress=%s strlen=%zd sz=%zd\n",buff,strlen(buff),sz);
    //printf("Intf0MacAddress=%s strlen=%zd sz=%zd\n",buff,strlen(buff),sz);
    memset(buff,0x00,sizeof(buff));
    strcpy(buff,"Intf1MacAddress=");
    //randomMac[1]=0xcc;
    //array2str(randomMac, strMac);
    strcat(buff+strlen(buff),mac1);
    buff[strlen(buff)]=0x0A;
    sz = write(fd,buff,strlen(buff));
    ALOGD("Intf1MacAddress=%s strlen=%zd sz=%zd\n",buff,strlen(buff),sz);
    //printf("Intf1MacAddress=%s strlen=%zd sz=%zd\n",buff,strlen(buff),sz);
    memset(buff,0x00,sizeof(buff));
    strcpy(buff,"END");
    buff[strlen(buff)]=0x0A;
    sz = write(fd,buff,strlen(buff));

    close(fd);

}

int main(int argc, char **argv)
{
    uint8_t randomMac[6];
    char mac0[20]={0},mac1[20]={0};
    if (argc==1)
    {
        ALOGD("Gen mac addr by rand");
        srand(time(NULL));
        randomMac[0] = (rand() & 0x0FF00000) >> 20;
        randomMac[1] = (rand() & 0x0FF00000) >> 20;
        randomMac[2] = (rand() & 0x0FF00000) >> 20;
        randomMac[3] = (rand() & 0x0FF00000) >> 20;
        randomMac[4] = (rand() & 0x0FF00000) >> 20;
        randomMac[5] = (rand() & 0x0FF00000) >> 20;
        randomMac[0] &= ~0x03;
        hex_to_bcd(randomMac, mac0, 6);
        randomMac[0] |= 0x02;
        hex_to_bcd(randomMac, mac1, 6);
        write_mac_to_file(mac0,mac1,false);
    }
    else if (argc==2)
    {
        unsigned char mac[6];
        ALOGD("Write mac addr %s",argv[1]);
        bcd_to_hex(argv[1],mac,6);
        mac[0] &= ~0x03;
        hex_to_bcd(mac,mac0,6);
        mac[0] |= 0x02;
        hex_to_bcd(mac,mac1,6);
        write_mac_to_file(mac0,mac1,true);
    }
    else
    {
        printf("parameter wrong\n");
        return -1;
    }
    chmod(WLAN_MAC_FILE,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    printf("success\n");
    return 0;
}


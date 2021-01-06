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
//#define NDEBUG 1

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
#define LOG_TAG "snwriter"
#endif
#define SN_FILE "/persist/odmsz.prop"


void write_sn_to_file(char *sn, bool force)
{
      int fd = 0;
      char buff[56] = {0};
      size_t sz = 0;
        if((access(SN_FILE,F_OK))!=-1&&(force!=true))
    {
        ALOGD("->SN_FILE exist\n");
        printf("->SN_FILE exist\n");
        return;
    }

    fd = open(SN_FILE,O_CREAT|O_RDWR,0644);
    if (fd<0)
    {
        ALOGD("->SN_FILE create failed fd=%d\n",fd);
        printf("->SN_FILE create failed fd=%d\n",fd);
        return;
    }

    strcpy(buff,"ro.boot.serialno=");
    //array2str(randomMac, strMac);
    strcat(buff+strlen(buff),sn);
    buff[strlen(buff)]=0x0A;
    sz = write(fd,buff,strlen(buff));
    ALOGD("->sn=%s strlen=%zd sz=%zd\n",buff,strlen(buff),sz);
    write(0,"success",strlen("success"));
    printf("success\n");
    close(fd);

    chmod(SN_FILE,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
}

int main(int argc, char **argv)
{
    if (argc!=2)
    {
        ALOGD("snwriter $serialno[8 bytes]\n");
        printf("snwriter $serialno[8 bytes]\n");
    }
    else 
    {
        write_sn_to_file(argv[1],true);
    }

    return 0;
}


#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <ctype.h>

#define MAX_ITEMS 20
#define MAX_LINE_LEN 256

typedef struct 
{
    char hardware[12];
    char platform[12];
    long pos;
}key_pos_t;

int main(int argc, char **argv)
{
    FILE *fp;
    char sensor_conf[] = "/system/etc/sensors/sensor_def_qcomdev.conf";

    key_pos_t key_pos_1984[MAX_ITEMS]={0},key_pos_1942[MAX_ITEMS]={0};
    int index_1984=0,index_1942=0;
    char platform[12] = {0}, hardware[12] = {0};

    char str_value_1984[5] = {0},str_value_1942[7] = {0};
    char line[MAX_LINE_LEN] = {0};
    int write = 0, i = 0;
    unsigned int near = 0, far = 0, value_1984 = 0,value_1942 = 0;
    if ((argc!=1)&&(argc!=3))
    {
        printf("Command input error\n");
        printf("sensor_regedit\n");
        printf("sensor_regedit near far\n");
        return 0;
    }

    if (argc==3)
    {
        write = 1;
        near = atoi(argv[1]);
        far = atoi(argv[2]);

        value_1984 = ((near>>8)<<4)|((far>>8));
        sprintf(str_value_1984,"%02x",value_1984);
        value_1942 = ((near&0xff)<<8)|(far&0xff);
        sprintf(str_value_1942,"%04x",value_1942);

        //printf("1984 %s\n",str_value_1984);
        //printf("1942 %s\n",str_value_1942);
    }

    system("mount -o rw,remount /system");
    fp = fopen(sensor_conf,"rw+");
    if (fp==NULL)
    {
        printf("%s open failed\n",sensor_conf);
        return -1;
    }
    setbuf(fp,NULL);
    while (fgets(line,256,fp)!=NULL)
    {
        if (!strncmp(line,":hardware ",10))
        {
            strcpy(hardware,line+10);
            hardware[strlen(hardware)-1]='\0';
            //printf("hardware %s\n",hardware);
        }                 
        else if (!strncmp(line,":platform ",10))
        {
            strcpy(platform,line+10);
            platform[strlen(platform)-1]='\0';
            //printf("platform %s\n",platform);
        }
        else if(!strncmp(line,"1942 ",5))
        {
            strcpy(key_pos_1942[index_1942].hardware,hardware);
            strcpy(key_pos_1942[index_1942].platform,platform);
            key_pos_1942[index_1942].pos = ftell(fp) - strlen(line);
            index_1942++;
        }
        else if(!strncmp(line,"1984 ",5))
        {
            strcpy(key_pos_1984[index_1984].hardware,hardware);
            strcpy(key_pos_1984[index_1984].platform,platform);
            key_pos_1984[index_1984].pos = ftell(fp) - strlen(line);
            index_1984++;
        }
    }
    for (i=0;i<MAX_ITEMS;i++)
    {
        if (!strcmp(key_pos_1942[i].hardware,"msm8996")&&!strcmp(key_pos_1942[i].platform,"QRD"))
        {
            fseek(fp,key_pos_1942[i].pos+7,SEEK_SET);
            if(write==0)
            {
                fgets(line,256,fp);
                //printf("1942 line=%s\n",line);
                value_1942 = strtol(line,NULL,16);

            }else
            {
                fwrite(str_value_1942,1,4,fp);
                //fgets(line,256,fp);
                //printf("line=%s\n",line);
            }
            
        }
        if (!strcmp(key_pos_1984[i].hardware,"msm8996")&&!strcmp(key_pos_1984[i].platform,"QRD"))
        {
            fseek(fp,key_pos_1984[i].pos+7,SEEK_SET);
            if(write==0)
            {
                fgets(line,256,fp);
                //printf("1984 line=%s\n",line);
                value_1984 = strtol(line,NULL,16);
            }else
            {
                fwrite(str_value_1984,1,2,fp);
                //fgets(line,256,fp);
                //printf("line=%s\n",line);
            }
            
        }
        //printf("1942 hardware %s :platform %s: pos = %ld\n",key_pos_1942[i].hardware,key_pos_1942[i].platform,key_pos_1942[i].pos);
        //printf("1984 hardware %s :platform %s: pos = %ld\n",key_pos_1984[i].hardware,key_pos_1984[i].platform,key_pos_1984[i].pos);
    }
    //printf("1942 %4x : 1984 %2x\n",value_1942,value_1984);
    near = ((value_1984&0xf0)<<4)|((value_1942&0xff00)>>8);
    far = ((value_1984&0x0f)<<8)|(value_1942&0x00ff); 
    printf("near %d\nfar %d\n",near,far);
    fclose(fp);
    system("rm /persist/sensors/sns.reg");
    system("mount -o ro,remount /system");
    return 0;
}
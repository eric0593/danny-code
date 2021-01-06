
#include "nvtool.h"
#include "nv.h"
#include <cutils/properties.h>


int imei_charArray_to_hexArray(unsigned char *input, int inputLen, int start, int len, unsigned char *output, int outputLen, bool revert) 
{
    int i = 0;

    if(inputLen < len || outputLen < len)
        return -1;

    for( i = 0; i < len; i++) 
    {  
        unsigned  char high = ((*(input + start + i))&0xf0)>>4;
        if(high >= 10)
            high = 'a' + high - 10;
        else
            high += '0';

        unsigned  char low = (*(input + start + i)&0x0f);
        if(low >= 10)
            low = 'a' + low - 10;
        else
            low += '0';

        if(revert) {
            if(i * 2 - 1>0)
                *(output + i * 2 - 1) = low;
            *(output + i * 2 ) = high;
        } else {
            if(i * 2 - 1>0)
                *(output + i * 2 -1) = high;
            *(output + i * 2) = low;
        }
    }
    return 0;
}

int imei_hexArray_to_charArray(unsigned char *input, int inputLen,unsigned char *output, int outputLen)
{
    int i = 0;

    if (inputLen<14||outputLen<8)
        return -1;
    output[0] = 0x08;
    output[1] = ((input[0] - '0')<<4)|0x0A;

    for (i=0;i<8;i++)
    {
        char low = input[1+2*i] - '0';
        char high = input[2+2*i] - '0';
        output[2+i] = (high <<4)|low;
    }
    return 0;
}

char get_imei_check_digit(unsigned char * imei)
{
    int         i;
    int         sum1=0,sum2=0,total=0;
    int         temp=0;

    for( i=0; i<14; i++ )
    {
        if((i%2)==0)
        {
            sum1 = sum1 + imei[i] - '0';
        }
        else
        {
            temp = (imei[i]-'0')*2;
            if( temp < 10 )
            {
                sum2 = sum2 + temp;
            }
            else
            {
                sum2 = sum2 + 1 + temp - 10;
            }
        }
    }

    total = sum1 + sum2;
    if( (total%10) == 0 )
    {
        return '0';
    }
    else
    {
        return (((total/10) * 10) + 10 - total+'0');
    }
}

void hex_to_bcd(unsigned char * hex, char * bcd, int len, bool revert)
{
    int i = 0;
    unsigned char low = 0, high = 0;

    for (i=0;i<len;i++)
    {
        low = (hex[i]&0x0f);
        high = ((hex[i]&0xf0)>>4);
        if (revert)
        {
            bcd[(len-i)*2-2] = high >=10? (high-10+'A'):(high+'0');
            bcd[(len-i)*2-1] = low >=10? (low-10+'A'):(low+'0');
        }
        else
        {
            bcd[2*i] = high >=10? (high-10+'A'):(high+'0');
            bcd[2*i+1] = low >=10? (low-10+'A'):(low+'0');
        }
    }
}

int bcd_to_hex(char * bcd, unsigned char * hex, int len,bool revert)
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

        if (revert)
        {
            hex[len-i-1] = (high<<4)+(low);
        }
        else
        {
            hex[i] = (high<<4)+(low);
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    unsigned char tmp[256] = { 0 };
    char format_buf[256] = { 0 };
    char nv_buf[256] = { 0 };
   // char sn[8] = {0};
    /* Calling LSM init  */
    if(!Diag_LSM_Init(NULL)) {
        printf("LSM init failed");
        return -1;
    }

    /* Register the callback for the primary processor */
    register_callback();

    if (argc==4||argc==3)
    {
        if ((!strcmp(argv[1],"read"))&&(!strcmp(argv[2],"imei")))
        {
            memset(format_buf, 0, sizeof(format_buf));
            diag_nv_read(NV_UE_IMEI_I, tmp, sizeof(tmp));
            usleep(NV_READ_DELAY);
            imei_charArray_to_hexArray(tmp, sizeof(tmp), 4, 8, (unsigned char *) format_buf, sizeof(format_buf), true);
            printf("IMEI=%s\n", format_buf);
        }else if ((!strcmp(argv[1],"read"))&&(!strcmp(argv[2],"bt")))
        {
            char bt_addr[13] = {0};
            memset(format_buf, 0, sizeof(format_buf));
            diag_nv_read(NV_BD_ADDR_I, tmp, sizeof(tmp));
            usleep(NV_READ_DELAY);
            strncpy(format_buf,(const char*)tmp+NV_DATA_OFFSET,6);
            hex_to_bcd((byte *)format_buf,bt_addr,6,true);
            printf("BT=%s\n", bt_addr);
        }
        else if((!strcmp(argv[1],"write"))&&(!strcmp(argv[2],"imei"))&&argc==4)
        {
            unsigned char imei15[16] = {0};
            int len = strlen(argv[3]);
            if (len>15||len<14)
            {
                printf("IMEI input wrong\n");
                return -1;
            }

            strcpy((char*)imei15, (char*)argv[3]);
            if (strlen(argv[3])==14)
            {
                imei15[14] = get_imei_check_digit(imei15);
            }
            memset(format_buf, 0, sizeof(format_buf));
            imei_hexArray_to_charArray(imei15,sizeof(imei15),(unsigned char *)format_buf,sizeof(format_buf));
            diag_nv_write(NV_UE_IMEI_I, (unsigned char *)format_buf, 9);
            printf("success\n");
        }else if ((!strcmp(argv[1],"write"))&&(!strcmp(argv[2],"bt"))&&argc==4)
        {
            byte bt_addr[6] = {0};
            memset(format_buf, 0, sizeof(format_buf));
            strcpy(format_buf,argv[3]);
            if (bcd_to_hex(format_buf, bt_addr, 6,true)!=0)
            {
                printf("failed\n");
                return -1;
            }
            diag_nv_write(NV_BD_ADDR_I, (unsigned char *)bt_addr, 6);
            printf("success\n");
        }
        goto exit;
    }

    memset(format_buf, 0, sizeof(format_buf));
    diag_nv_read(NV_UE_IMEI_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    imei_charArray_to_hexArray(tmp, sizeof(tmp), 4, 8, (unsigned char *) format_buf, sizeof(format_buf), true);
    printf("IMEI=%s\n", format_buf);
/*  spc failed
    memset(format_buf, 0, sizeof(format_buf));
    diag_nv_read(NV_DEVICE_SERIAL_NO_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    printf("NV_DEVICE_SERIAL_NO_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "NV_DEVICE_SERIAL_NO_I = %s\n", tmp+3);
    strncpy(sn,(const char*)tmp+NV_DATA_OFFSET,7);
    printf("%s\n", sn);
    property_set("ro.device.id", sn);
*/
    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_FACTORY_DATA_1_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_FACTORY_DATA_1_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_FACTORY_DATA_1_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_FACTORY_DATA_2_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_FACTORY_DATA_2_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_FACTORY_DATA_2_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_FACTORY_DATA_3_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_FACTORY_DATA_3_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_FACTORY_DATA_3_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_FACTORY_DATA_4_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_FACTORY_DATA_1_4 %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_FACTORY_DATA_4_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_1_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_1_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_1_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_2_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_2_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_2_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_3_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_3_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_3_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_4_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_4_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_4_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_5_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_5_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_5_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_6_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_6_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d:",NV_OEM_ITEM_6_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_7_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_7_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%0d:",NV_OEM_ITEM_7_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);

    memset(tmp, 0, sizeof(tmp));
    diag_nv_read(NV_OEM_ITEM_8_I, tmp, sizeof(tmp));
    usleep(NV_READ_DELAY);
    //printf("NV_OEM_ITEM_8_I %x %x %x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4]);
    snprintf(format_buf, sizeof(format_buf), "%d=%d",NV_OEM_ITEM_8_I, tmp[NV_DATA_OFFSET]);
    strlcat(nv_buf, format_buf, sizeof(nv_buf));
    printf("%s\n", format_buf);
    property_set("ro.oem.item", nv_buf);

exit:

    Diag_LSM_DeInit();
    return 0;

}

#define LOG_TAG "i2c_controller"

#include <stdio.h>
#include <stdlib.h>
#include "utils/Log.h"
#include "fpga_loader.h"
#include <fcntl.h>

#include "i2c_main.h"

void sendRefreshCommand()
{
	int fd = open("/dev/i2c_operator_device", O_RDWR);
	if(NULL == fd)
	{
		printf("Timothy:open device error");
		return;
	}

	int data_length = 2*sizeof(unsigned char);
	unsigned char *data = new unsigned char[data_length];	
	memset(data, 0x00, data_length);
	data[0] = 0; // device_index
	data[1] = 2; // flag:means send refresh command
	
	write(fd, data, data_length);

	close(fd);
}

void load_fpga_firmware(JNIEnv *env, jobject object)
{
	printf("start to load fpga firmware");
	if(load( "/system/etc/smart/pro_fea_data_header.iea", "/system/etc/smart/pro_fea_data_header.ied", 0) < 0)
	{
		printf("first down load fail");
		return;
	}

	sleep(1);

	sendRefreshCommand();

	sleep(1);
	
	if(load( "/sdcard/pro_flash_algo.iea", "/sdcard/pro_flash_data.ied", 1) < 0)
	{
		printf("second down load fail");	
		return;
	}
}

int main()
{
	printf("Step 1: start to load fpga header\n");
	if(load( "/system/etc/smartbox/pro_fea_algo_header.iea", "/system/etc/smartbox/pro_fea_data_header.ied", 0) < 0)
	{
		printf("header load fail\n");
		return -1;
	}

	sleep(1);

	sendRefreshCommand();

	sleep(1);
	printf("Step 2: start to load fpga firmware\n");
	if(load( "/sdcard/pro_flash_algo.iea", "/sdcard/pro_flash_data.ied", 1) < 0)
	{
		printf("firmware load fail\n");			
		printf("Can not find file here\n");			
		printf("/sdcard/pro_flash_algo.iea\n");	
		printf("/sdcard/pro_flash_data.ied\n");			
		return -1;
	}
	return 0;
}

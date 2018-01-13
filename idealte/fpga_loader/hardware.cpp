/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2011
* 
*
***************************************************************/


/**************************************************************
* 
* Revision History of hardware.c
* 
* 
* Support v1.0
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include "opcode.h"
#include <utils/Log.h>
#include "i2c_controller_utils.h"

#define LOG_TAG "i2c_controller"

/*************************************************************
*                                                            *
* Opcode for discrete pins toggling							 *
*                                                            *
*************************************************************/
//#define signalSCL		  0x01    //I2C Bus
//#define signalSDA		  0x08    
//#define signalSCL_IN    0x0003    
//#define signalSDA_IN	  0x0004    

/********************************************************************************
* Declaration of global variables 
*
*********************************************************************************/
unsigned short g_usCpu_Frequency  = 1000;   /*Enter your CPU frequency here, unit in MHz.*/
static int global_tmp;
static unsigned char data_send[1024];
static int data_send_length;
int fd;

/***************************************************************
*
* Functions declared in hardware.c module.
*
***************************************************************/
int ReadBytesAndSendNACK( int length, unsigned char *a_ByteRead, int NAck );
int SendBytesAndCheckACK(int length, unsigned char *a_bByteSend);
int SetI2CStopCondition();

void EnableHardware(int type);
void DisableHardware();
void SetI2CDelay( unsigned int a_msTimeDelay, int type );

/*************************************************************
*                                                            *
* SetI2CStartCondition                                       *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     int                                                   *
*                                                            *
* DESCRIPTION:                                               *
*     This function is used to issue a start sequence on the *
*     I2C Bus								                 *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
int SetI2CStartCondition(int type)
{
	printf("Timothy:hardware.cpp->SetI2CStartCondition()");
	if(type == 0)
	{
		sleep(1);
	}
	//SCL SDA high
	//signalSDA = 1;
	//SetI2CDelay(1);
	//signalSCL = 1;
	//SetI2CDelay(1);
	//SCL high SDA low
	//signalSDA = 0;
	//SetI2CDelay(1);
	//SCL low SDA low
	//signalSCL = 0;
	//SetI2CDelay(1);
	
	return 0;
}
/*************************************************************
*                                                            *
* SetI2CReStartCondition                                     *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     None                                                   *
*                                                            *
* DESCRIPTION:                                               *
*     This function is used to issue a start sequence on the *
*     I2C Bus								                 *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
int SetI2CReStartCondition()
{
//	printf("Timothy:hardware.cpp->SetI2CReStartCondition()");
	if(data_send_length == 0)
	{
//		printf("Timothy:data is null");
		return 0;
	}

	int data_length = 2*sizeof(unsigned char) + data_send_length*sizeof(unsigned char);
	unsigned char *data = new unsigned char[data_length];	
	memset(data, 0x00, data_length);
	data[0] = I2C_DEVICE_FPGA; // device_index
	data[1] = 1; // flag:means do not send stop in the end
	for(int i = 0; i < data_send_length; i++)
	{
		data[2+i] = data_send[i];
	}
	
	// clear data buf
	data_send_length = 0;
	memset(data_send, 0x00, 1024);
	
	int count = write(fd, data, data_length);
	if(count != data_length - 2)
	{
		printf("Timothy:not write enough, count = %d, data_send_length = %d", count, data_length);
		return -1;
	}
	else
	{
//		printf("Timothy:write success:%d", count);
	}
	delete []data;
//	printf("Timothy:finish free memory");
	return 0;

	return 0;
	
}
/*************************************************************
*                                                            *
* SetI2CStopCondition                                        *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     None                                                   *
*                                                            *
* DESCRIPTION:                                               *
*     This function is used to issue a stop sequence on the  *
*     I2C Bus								                 *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
int SetI2CStopCondition()
{
//	printf("Timothy:hardware.cpp->SetI2CStopCondition()");

	if(data_send_length == 0)
	{
//		printf("Timothy:data is null");
		return 0;
	}

	int data_length = 2*sizeof(unsigned char) + data_send_length*sizeof(unsigned char);
	unsigned char *data = new unsigned char[data_length];	
	memset(data, 0x00, data_length);
	data[0] = I2C_DEVICE_FPGA; // device_index
	data[1] = 0; // flag:means send stop in the end
	for(int i = 0; i < data_send_length; i++)
	{
		data[2+i] = data_send[i];
	}
	
	// clear data buf
	data_send_length = 0;
	memset(data_send, 0x00, 1024);
	
	int count = write(fd, data, data_length);
	if(count != data_length - 2)
	{
		printf("Timothy:not write enough, count = %d, data_send_length = %d", count, data_length);
		return -1;
	}
	else
	{
//		printf("Timothy:write success:%d", count);
	}
	delete []data;
//	printf("Timothy:finish free memory");
	return 0;
}

/*************************************************************
*                                                            *
* READBYTESANDSENDNACK                                         *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     Returns the bit read back from the device.             *
*                                                            *
* DESCRIPTION:                                               *
*     This function is used to read the TDO pin from the     *
*     input port.                                            *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
int ReadBytesAndSendNACK( int old_length, unsigned char *a_ByteRead , int NAck)
{
//	printf("Timothy:hardware.cpp->ReadBytesAndSendNACK(), length = %d", old_length);
	int i;
	int length;
	length = old_length / 8; // 重新计算字节数
	if(old_length%8 != 0)
	{
		length += 1;
	}
//	printf("Timothy:now the length is %d", length);
	int data_length = sizeof(unsigned char) + length*sizeof(unsigned char);
	unsigned char *data = new unsigned char[data_length];
	memset(data, 0x00, data_length);
	data[0] = I2C_DEVICE_FPGA; // device_index
	
	int count = read(fd, data, data_length);
	if(count != length)
	{
		printf("Timothy:not read enough, count = %d, length = %d", count, length);
		return 0;
	}
	else
	{
//		printf("Timothy:read success:%d", count);
	}
	for(i = 0; i < length; i++)
	{
		a_ByteRead[i] = data[1+i];
	}

	delete []data;
//	printf("Timothy:finish free memory");
//	printf("Timothy:the data read is:");
//	for(int i = 0; i < length; i++)
//	{
//		printf("%x", a_ByteRead[i]);
//	}
	return 0;
}
/*************************************************************
*                                                            *
* SENDBYTESANDCHECKACK                                       *
*                                                            *
* INPUT:                                                     *
*                                                            *
*     a_bByteSend: the value to determine of the pin above   *
*     will be written out or not.                            *
*                                                            *
* RETURN:                                                    *
*     true or false.                                         *
*                                                            *
* DESCRIPTION:                                               *
*     To apply the specified value to the pins indicated.    *
*     This routine will likely be modified for specific      *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
int SendBytesAndCheckACK(int old_length, unsigned char *a_bByteSend)
{	
//	printf("Timothy:hardware.cpp->SendBytesAndCheckACK(), length = %d", old_length);
	if((old_length == 8 && a_bByteSend[0] == 0x80) ||
		(old_length == 8 && a_bByteSend[0] == 0x81))
	{
//		printf("Timothy:this time is address, pass");
		return 0;
	}
	int length;
	int i;
	length = old_length/8; // 重新计算字节数
	if(old_length%8 != 0)
	{
		length += 1;
	}
	for(i = 0; i < length; i++)
	{
		data_send[data_send_length+i] = a_bByteSend[i];
	}
	data_send_length += length;

	
//	printf("Timothy:the data will write is:");
//	for(int i = 0; i < length; i++)
//	{
//		printf("%x", a_bByteSend[i]);
//	}
	
	return 0;
	/*
	int length;
	length = old_length/8; // 重新计算字节数
	if(old_length%8 != 0)
	{
		length+=1;
	}
	printf("Timothy:now the length is %d", length);	
	printf("Timothy:the data write is:");
	for(int i = 0; i < length; i++)
	{
		printf("%x", a_bByteSend[i]);
	}
	int data_length = sizeof(unsigned char) + length*sizeof(unsigned char);
	unsigned char *data = new unsigned char[data_length];	
	memset(data, 0x00, data_length);
	data[0] = I2C_DEVICE_FPGA; // device_index
	for(int i = 0; i < length; i++)
	{
		data[1+i] = a_bByteSend[i];
	}
	int fd = open("/dev/i2c_operator_device", O_RDWR);
	if(NULL == fd)
	{
		printf("Timothy:open device error");
		return -1;
	}
	int count = write(fd, data, data_length);
	if(count != length)
	{
		printf("Timothy:not write enough, count = %d, length = %d", count, length);
		return -1;
	}
	else
	{
		printf("Timothy:write success:%d", count);
	}
	close(fd);
	delete []data;
	printf("Timothy:finish free memory");
	return 0;
	*/
}

/*************************************************************
*                                                            *
* SetI2CDelay                                                *
*                                                            *
* INPUT:                                                     *
*     a_uiDelay: number of waiting time.                     *
*                                                            *
* RETURN:                                                    *
*     None.                                                  *
*                                                            *
* DESCRIPTION:                                               *
*     Users must devise their own timing procedures to       *
*     ensure the specified minimum delay is observed when    *
*     using different platform.  The timing function used    *
*     here is for PC only by hocking the clock chip.         *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/
void SetI2CDelay( unsigned int a_msTimeDelay )
{
	printf("Timothy:hardware.cpp->SetI2CDelay()");
//	sleep(6);	
	unsigned short loop_index     = 0;
	unsigned short ms_index       = 0;
	unsigned short us_index       = 0;

	/*Users can replace the following section of code by their own*/
	for( ms_index = 0; ms_index < a_msTimeDelay; ms_index++)
	{
		/*Loop 1000 times to produce the milliseconds delay*/
		for (us_index = 0; us_index < 1000; us_index++)
		{ /*each loop should delay for 1 microsecond or more.*/
			loop_index = 0;
			do {
				/*The NOP fakes the optimizer out so that it doesn't toss out the loop code entirely*/
//				__asm NOP
				global_tmp = us_index;
			}while (loop_index++ < ((g_usCpu_Frequency/8)+(+ ((g_usCpu_Frequency % 8) ? 1 : 0))));/*use do loop to force at least one loop*/
		}
	}
}
/*************************************************************
*                                                            *
* ENABLEHARDWARE                                             *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     None.                                                  *
*                                                            *
* DESCRIPTION:                                               *
*     This function is called to enable the hardware.        *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/

void EnableHardware(int type)
{
//	printf("Timothy:hardware.cpp->EnableHardware()");
	
	fd = open("/dev/i2c_operator_device", O_RDWR);
	if(NULL == fd)
	{
		printf("Timothy:open device error");
		return;
	}
	
	data_send_length = 0;
	SetI2CStartCondition(type);
	SetI2CStopCondition();
}

/*************************************************************
*                                                            *
* DISABLEHARDWARE                                            *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     None.                                                  *
*                                                            *
* DESCRIPTION:                                               *
*     This function is called to disable the hardware.       *
*                                                            *
*     NOTE: This function should be modified in an embedded  *
*     system!                                                *
*                                                            *
*************************************************************/

void DisableHardware()
{
//	printf("Timoth:hardware.cpp->DisableHardware()");
	data_send_length = 0;
	SetI2CStopCondition();

//	int data_length = 2*sizeof(unsigned char);
//	unsigned char *data = new unsigned char[data_length];	
//	memset(data, 0x00, data_length);
//	data[0] = I2C_DEVICE_FPGA; // device_index
//	data[1] = 2; // flag:means send refresh command
//	
//	write(fd, data, data_length);

//	sleep(1);

//	data[0] = I2C_DEVICE_FPGA; // device_index
//	data[1] = 3; // flag:means send refresh command
//	
//	write(fd, data, data_length);

//	delete []data;
	
	close(fd);
}


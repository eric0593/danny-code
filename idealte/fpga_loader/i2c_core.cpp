/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2011
* 
*
***************************************************************/


/**************************************************************
* 
* Revision History of i2c_core.c
* 
* 
* Support version 1.0
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "opcode.h"
#include <utils/Log.h>
#define LOG_TAG "i2c_controller"

#define I2C_DEBUG 1


/*************************************************************
*                                                            *
* PROTOTYPES                                                 *
*                                                            *
*************************************************************/

unsigned int ispVMDataSize();
short int ispVMShiftExec( unsigned int a_uiDataSize );
short int ispVMShift( char a_cCommand );
unsigned char GetByte( int a_iCurrentIndex, char a_cAlgo );
short int ispVMRead( unsigned int a_uiDataSize );
short int  ispVMSend( unsigned int a_uiDataSize );
void ispVMComment();
short int ispVMLoop( unsigned short a_usLoopCount, int type );

extern int SetI2CStartCondition(int type);
extern int SetI2CReStartCondition();
extern int SetI2CStopCondition();
extern void SetI2CDelay( unsigned int a_usDelay );
extern int SendBytesAndCheckACK(int length, unsigned char *a_bByteSend);
extern int ReadBytesAndSendNACK( int length, unsigned char *a_ByteRead, int NAck );

/*************************************************************
*                                                            *
* GLOBAL VARIABLES                                           *
*                                                            *
*************************************************************/

unsigned short g_usDataType = 0x0000; /*** data type register used to hold information ***/
int g_iMovingAlgoIndex = 0;	    /*** variable to hold the current index in the algo array ***/
int g_iMovingDataIndex = 0;		/*** variable to hold the current index in the data array ***/
int g_iMainDataIndex = 0;		/*** forward-only index used as a placed holder in the data array ***/
int g_iRepeatIndex = 0;		    /*** Used to point to the location of REPEAT data ***/
int g_iTDIIndex = 0;			/*** Used to point to the location of TDI data ***/
int g_iTDOIndex = 0;			/*** Used to point to the location of TDO data ***/
int g_iMASKIndex = 0;			/*** Used to point to the location of MASK data ***/
unsigned char g_ucCompressCounter = 0; /*** used to indicate how many times 0xFF is repeated ***/

/*************************************************************
*                                                            *
* EXTERNAL VARIABLES                                         *
*                                                            *
*************************************************************/
extern unsigned char * g_pucAlgoArray;
extern unsigned char * g_pucDataArray;
extern int g_iAlgoSize;
extern int g_iDataSize;
int  g_iLoopIndex = 0;
int  g_iLoopMovingIndex = 0;	
int  g_iLoopDataMovingIndex = 0;	 
unsigned short g_usLCOUNTSize	= 0;


/*************************************************************
*                                                            *
* ISPPROCESSI2C                                              *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     The return value indicates whether the i2c was         *
*     processed successfully or not.  A return value equal   *
*     to or greater than 0 is passing, and less than 0 is    *
*     failing.                                               *
*                                                            *
* DESCRIPTION:                                               *
*     This function is the core of the embedded processor.   *
*************************************************************/
short int ispProcessI2C(int type)
{
	unsigned char ucOpcode  = 0;
	unsigned char ucState   = 0;
	short int siRetCode     = 0;
	static char cProgram    = 0;
	unsigned int uiDataSize = 0;
	int iLoopCount          = 0;
	unsigned int iMovingAlgoIndex = 0;
	int intDelay            = 0;
	
	/*************************************************************
	*                                                            *
	* Begin processing the I2C algorithm and data files.         *
	*                                                            *
	*************************************************************/
	
	while ( ( ucOpcode = GetByte( g_iMovingAlgoIndex++, 1 ) ) != 0xFF ) 
	{
		/*************************************************************
		*                                                            *
		* This switch statement is the main switch that represents   *
		* the core of the embedded processor.                        *
		*                                                            *
		*************************************************************/
		
		switch ( ucOpcode ) 
		{
		case I2C_STARTTRAN:
#ifdef I2C_DEBUG
			printf("Start Condition\n");
#endif /* I2C_DEBUG */
			siRetCode = SetI2CStartCondition(type);
			break;
		case I2C_RESTARTTRAN:
#ifdef I2C_DEBUG
			printf("ReStart Condition\n");
#endif /* I2C_DEBUG */
			siRetCode = SetI2CReStartCondition();
			break;
		case I2C_ENDTRAN:
#ifdef I2C_DEBUG
			printf("Stop Condition\n");
#endif /* I2C_DEBUG */
			siRetCode = SetI2CStopCondition();
			break;
		case I2C_TRANSOUT:
		case I2C_TRANSIN:
			/*************************************************************
			*                                                            *
			* Execute SIR/SDR command.                                   *
			*                                                            *
			*************************************************************/
			siRetCode = ispVMShift( ucOpcode );
			break;		
		case I2C_WAIT:
			/*************************************************************
			*                                                            *
			* Issue delay in specified time.                             *
			*                                                            *
			*************************************************************/
			intDelay = ispVMDataSize();
#ifdef I2C_DEBUG
			printf("Delay %d\n", intDelay);
#endif /* I2C_DEBUG */
			SetI2CDelay( intDelay );
			break;
		case I2C_BEGIN_REPEAT:
			/*************************************************************
			*                                                            *
			* Execute repeat loop.                                       *
			*                                                            *
			*************************************************************/

			uiDataSize = ispVMDataSize();

			switch ( GetByte( g_iMovingAlgoIndex++, 1 ) ) {
			case I2C_PROGRAM:
				/*************************************************************
				*                                                            *
				* Set the main data index to the moving data index.  This    *
				* allows the processor to remember the beginning of the      *
				* data.  Set the cProgram variable to true to indicate to    *
				* the verify flow later that a programming flow has been     *
				* completed so the moving data index must return to the      *
				* main data index.                                           *
				*                                                            *
				*************************************************************/
				g_iMainDataIndex = g_iMovingDataIndex;
				cProgram = 1;
				break;
			case I2C_VERIFY:
				/*************************************************************
				*                                                            *
				* If the static variable cProgram has been set, then return  *
				* the moving data index to the main data index because this  *
				* is a erase, program, verify operation.  If the programming *
				* flag is not set, then this is a verify only operation thus *
				* no need to return the moving data index.                   *
				*                                                            *
				*************************************************************/
				if ( cProgram ) {
					g_iMovingDataIndex = g_iMainDataIndex;
					cProgram = 0;
				}
				break;
			}

			/*************************************************************
			*                                                            *
			* Set the repeat index to the first byte in the repeat loop. *
			*                                                            *
			*************************************************************/

			g_iRepeatIndex = g_iMovingAlgoIndex;

			for ( ; uiDataSize > 0; uiDataSize-- ) {
				/*************************************************************
				*                                                            *
				* Initialize the current algorithm index to the beginning of *
				* the repeat index before each repeat loop.                  *
				*                                                            *
				*************************************************************/

				g_iMovingAlgoIndex = g_iRepeatIndex;

				/*************************************************************
				*                                                            *
				* Make recursive call.                                       *
				*                                                            *
				*************************************************************/

				siRetCode = ispProcessI2C(type);
				if ( siRetCode < 0 ) {
					break;
				}
			}
			break;
		case I2C_END_REPEAT:
			/*************************************************************
			*                                                            *
			* Exit the current repeat frame.                             *
			*                                                            *
			*************************************************************/
			return siRetCode;
			break;
		case I2C_LOOP:
			/*************************************************************
			*                                                            *
			* Execute repeat loop.                                       *
			*                                                            *
			*************************************************************/

			g_usLCOUNTSize = (short int)ispVMDataSize();			

#ifdef I2C_DEBUG
			printf( "LoopCount %d\n", g_usLCOUNTSize );
#endif
			siRetCode = ispVMLoop( ( unsigned short ) g_usLCOUNTSize, type );
			if ( siRetCode != 0 ) {
				return ( siRetCode );
			}			
			break;
		case I2C_ENDLOOP:
			/*************************************************************
			*                                                            *
			* Exit the current repeat frame.                             *
			*                                                            *
			*************************************************************/			
			break;
		case I2C_COMMENT:
			ispVMComment();
			break;
		case I2C_ENDVME:
			/*************************************************************
			*                                                            *
			* If the ENDVME token is found and g_iMovingAlgoIndex is     *
			* greater than or equal to g_iAlgoSize, then that indicates  *
			* the end of the chain.  If g_iMovingAlgoIndex is less than  *
			* g_iAlgoSize, then that indicates that there are still more *
			* devices to be processed.                                   *
			*                                                            *
			*************************************************************/
			if ( g_iMovingAlgoIndex >= g_iAlgoSize ) {
				return siRetCode;
			}
			break;
		default:
			/*************************************************************
			*                                                            *
			* Unrecognized opcode.  Return with file error.              *
			*                                                            *
			*************************************************************/
			return ERR_ALGO_FILE_ERROR;
		}
		
		if ( siRetCode < 0 ) {
			return siRetCode;
		}
	}	
	return ERR_ALGO_FILE_ERROR;
}

/*************************************************************
*                                                            *
* ISPVMDATASIZE                                              *
*                                                            *
* INPUT:                                                     *
*     None.                                                  *
*                                                            *
* RETURN:                                                    *
*     This function returns a number indicating the size of  *
*     the instruction.                                       *
*                                                            *
* DESCRIPTION:                                               *
*     This function returns a number.  
*************************************************************/
unsigned int ispVMDataSize()
{
	unsigned int uiSize = 0;
	unsigned char ucCurrentByte = 0;
	unsigned char ucIndex = 0;
	
	while ( ( ucCurrentByte = GetByte( g_iMovingAlgoIndex++, 1 ) ) & 0x80 ) {
		uiSize |= ( ( unsigned int ) ( ucCurrentByte & 0x7F ) ) << ucIndex;
		ucIndex += 7;
	}
	uiSize |= ( ( unsigned int ) ( ucCurrentByte & 0x7F ) ) << ucIndex;
	return uiSize;
}

/*************************************************************
*                                                            *
* ISPVMSHIFTEXEC                                             *
*                                                            *
* INPUT:                                                     *
*     a_uiDataSize: this holds the size of the command.      *
*                                                            *
* RETURN:                                                    *
*     Returns 0 if passing, -1 if failing.                   *
*                                                            *
* DESCRIPTION:                                               *
*     This function handles the data in the commands         *
*     by either decompressing the data or setting the        *
*     respective indexes to point to the appropriate         *
*     location in the algo or data array.                    *
*                                                            *
*************************************************************/
short int ispVMShiftExec( unsigned int a_uiDataSize )
{
	unsigned char ucDataByte = 0;
	
	/*************************************************************
	*                                                            *
	* Reset the data type register.                              *
	*                                                            *
	*************************************************************/

	g_usDataType &= ~( TDI_DATA + TDO_DATA + MASK_DATA + DTDI_DATA + DTDO_DATA + COMPRESS_FRAME );

	/*************************************************************
	*                                                            *
	* Convert the size from bits to byte.                        *
	*                                                            *
	*************************************************************/

	if ( a_uiDataSize % 8 ) {
		a_uiDataSize = a_uiDataSize / 8 + 1;
	}
	else {
		a_uiDataSize = a_uiDataSize / 8;
	}
	
	/*************************************************************
	*                                                            *
	* Begin extracting the command.                              *
	*                                                            *
	*************************************************************/

	while ( ( ucDataByte = GetByte( g_iMovingAlgoIndex++, 1 ) ) != I2C_CONTINUE ) { 
		switch ( ucDataByte ) {
		case I2C_TDI:
			/*************************************************************
			*                                                            *
			* Set data type register to indicate TDI data and set TDI    *
			* index to the current algorithm location.                   *
			*                                                            *
			*************************************************************/
			g_usDataType |= TDI_DATA;
			g_iTDIIndex = g_iMovingAlgoIndex;
			g_iMovingAlgoIndex += a_uiDataSize;
			break;
		case I2C_DTDI:
			/*************************************************************
			*                                                            *
			* Set data type register to indicate DTDI data and check the *
			* next byte to make sure it's the DATA byte.  DTDI indicates *
			* that the data should be read from the data array, not the  *
			* algo array.                                                *
			*                                                            *
			*************************************************************/
			g_usDataType |= DTDI_DATA;
			if ( GetByte( g_iMovingAlgoIndex++, 1 ) != I2C_DATA ) {
				return ERR_ALGO_FILE_ERROR;
			}

			/*************************************************************
			*                                                            *
			* If the COMPRESS flag is set, read the next byte from the   *
			* data file array.  If the byte is true, then that indicates *
			* the frame was compressable.  Note that even though the     *
			* overall data file was compressed, certain frames may not   *
			* be compressable that is why this byte must be checked.     *
			*                                                            *
			*************************************************************/
			if ( g_usDataType & COMPRESS ) {
				if ( GetByte( g_iMovingDataIndex++, 0 ) ) {
					g_usDataType |= COMPRESS_FRAME;
				}
			}
			break;
		case I2C_TDO:
			/*************************************************************
			*                                                            *
			* Set data type register to indicate TDO data and set TDO    *
			* index to the current algorithm location.                   *
			*                                                            *
			*************************************************************/
			g_usDataType |= TDO_DATA;
			g_iTDOIndex = g_iMovingAlgoIndex;
			g_iMovingAlgoIndex += a_uiDataSize;
			break;
		case I2C_DTDO:
			/*************************************************************
			*                                                            *
			* Set data type register to indicate DTDO data and check the *
			* next byte to make sure it's the DATA byte.  DTDO indicates *
			* that the data should be read from the data array, not the  *
			* algo array.                                                *
			*                                                            *
			*************************************************************/
			g_usDataType |= DTDO_DATA;
			if ( GetByte( g_iMovingAlgoIndex++, 1 ) != I2C_DATA ) {
				return ERR_ALGO_FILE_ERROR;
			}

			/*************************************************************
			*                                                            *
			* If the COMPRESS flag is set, read the next byte from the   *
			* data file array.  If the byte is true, then that indicates *
			* the frame was compressable.  Note that even though the     *
			* overall data file was compressed, certain frames may not   *
			* be compressable that is why this byte must be checked.     *
			*                                                            *
			*************************************************************/
			
			if ( g_usDataType & COMPRESS ) {
				if ( !(g_usDataType & DTDI_DATA) ) {
					if ( GetByte( g_iMovingDataIndex++, 0 ) ) {
						g_usDataType |= COMPRESS_FRAME;
					}
				}
			}
			break;
		case I2C_MASK:
			/*************************************************************
			*                                                            *
			* Set data type register to indicate MASK data.  Set MASK    *
			* location index to current algorithm array position.        *
			*                                                            *
			*************************************************************/
			g_usDataType |= MASK_DATA;
			g_iMASKIndex = g_iMovingAlgoIndex;
			g_iMovingAlgoIndex += a_uiDataSize;
			break;
		default:
			/*************************************************************
			*                                                            *
			* Unrecognized or misplaced opcode.  Return error.           *
			*                                                            *
			*************************************************************/
			return ERR_ALGO_FILE_ERROR;
		}
	}  
	
	/*************************************************************
	*                                                            *
	* Reached the end of the instruction.  Return passing.       *
	*                                                            *
	*************************************************************/

	return 0;
}

/*************************************************************
*                                                            *
* ISPVMSHIFT                                                 *
*                                                            *
* INPUT:                                                     *
*     a_cCommand: this argument specifies either the SIR or  *
*     SDR command.                                           *
*                                                            *
* RETURN:                                                    *
*     The return value indicates whether the SIR/SDR was     *
*     processed successfully or not.  A return value equal   *
*     to or greater than 0 is passing, and less than 0 is    *
*     failing.                                               *
*                                                            *
* DESCRIPTION:                                               *
*     This function is the entry point to execute an SIR or  *
*     SDR command to the device.                             *
*                                                            *
*************************************************************/
short int ispVMShift( char a_cCommand )
{
	short int siRetCode = 0;
	unsigned int uiDataSize = ispVMDataSize();

#ifdef I2C_DEBUG
	printf( "SDR %d ", uiDataSize );
#endif /* I2C_DEBUG */
	
	/*************************************************************
	*                                                            *
	* Clear any existing SDR instructions from the data type *
	* register.                                                  *
	*                                                            *
	*************************************************************/
	
	g_usDataType &= ~( SDR_DATA );

	/*************************************************************
	*                                                            *
	* Set the data type register to indicate that it's executing *
	* an SDR instruction.  Move state machine to DRPAUSE,        *
	* SHIFTDR.  If header data register exists, then issue       *
	* bypass.                                                    *
	*                                                            *
	*************************************************************/
	g_usDataType |= SDR_DATA;		
		
	/*************************************************************
	*                                                            *
	* Set the appropriate index locations.  If error then return *
	* error code immediately.                                    *
	*                                                            *
	*************************************************************/

	siRetCode = ispVMShiftExec( uiDataSize );
	
	if ( siRetCode < 0 ) {
		return siRetCode;
	}
	/*************************************************************
	*                                                            *
	* Execute the command to the device.  If TDO exists, then    *
	* read from the device and verify.  Else only TDI exists     *
	* which must send data to the device only.                   *
	*                                                            *
	*************************************************************/

	if ( ( g_usDataType & TDO_DATA ) || ( g_usDataType & DTDO_DATA ) ) {
		siRetCode = ispVMRead( uiDataSize );
		/*************************************************************
		*                                                            *
		* A frame of data has just been read and verified.  If the   *
		* DTDO_DATA flag is set, then check to make sure the next    *
		* byte in the data array, which is the last byte of the      *
		* frame, is the END_FRAME byte.                              *
		*                                                            *
		*************************************************************/
		if ( g_usDataType & DTDO_DATA ) {
			if ( GetByte( g_iMovingDataIndex++, 0 ) != I2C_END_FRAME ) {
				siRetCode = ERR_DATA_FILE_ERROR;
			}
		}
	}
	else {
		siRetCode = ispVMSend( uiDataSize );
		/*************************************************************
		*                                                            *
		* A frame of data has just been sent.  If the DTDI_DATA flag *
		* is set, then check to make sure the next byte in the data  *
		* array, which is the last byte of the frame, is the         *
		* END_FRAME byte.                                            *
		*                                                            *
		*************************************************************/
		if ( g_usDataType & DTDI_DATA ) {
			if ( GetByte( g_iMovingDataIndex++, 0 ) != I2C_END_FRAME ) {
				siRetCode = ERR_DATA_FILE_ERROR;
			}
		}
	}	
	return siRetCode;
}
/*************************************************************
*                                                            *
* GETBYTE                                                    *
*                                                            *
* INPUT:                                                     *
*     a_iCurrentIndex: the current index to access.          *
*                                                            *
*     a_cAlgo: 1 if the return byte is to be retrieved from  *
*     the algorithm array, 0 if the byte is to be retrieved  *
*     from the data array.                                   *
*                                                            *
* RETURN:                                                    *
*     This function returns a byte of data from either the   *
*     algorithm or data array.  It returns -1 if out of      *
*     bounds.                                                *
*                                                            *
*************************************************************/
unsigned char GetByte( int a_iCurrentIndex, char a_cAlgo )
{
	unsigned char ucData = 0;
	
	if ( a_cAlgo ) { 
		/*************************************************************
		*                                                            *
		* If the current index is still within range, then return    *
		* the next byte.  If it is out of range, then return -1.     *
		*                                                            *
		*************************************************************/
		if ( a_iCurrentIndex >= g_iAlgoSize ) {
			return ( unsigned char ) 0xFF;
		}
		ucData = g_pucAlgoArray[ a_iCurrentIndex ];
	}
	else {
		/*************************************************************
		*                                                            *
		* If the current index is still within range, then return    *
		* the next byte.  If it is out of range, then return -1.     *
		*                                                            *
		*************************************************************/
		if ( a_iCurrentIndex >= g_iDataSize ) {
			return ( unsigned char ) 0xFF;
		}
		ucData = g_pucDataArray[ a_iCurrentIndex ];
	}

	return ucData;
}
/*************************************************************
*                                                            *
* ISPVMREAD                                                  *
*                                                            *
* INPUT:                                                     *
*     a_uiDataSize: this argument is the size of the         *
*     command.                                               *
*                                                            *
* RETURN:                                                    *
*     The return value is 0 if passing, and -1 if failing.   *
*                                                            *
* DESCRIPTION:                                               *
*     This function reads a data stream from the device and  *
*     compares it to the expected TDO.                       *
*                                                            *
*************************************************************/
short int ispVMRead( unsigned int a_uiDataSize )
{
	unsigned int uiIndex = 0;
	unsigned int usBufferIndex = 0;
	unsigned short usErrorCount = 0;
	unsigned char ucTDOByte = 0;
	unsigned char ucTDIByte = 0;
	unsigned char ucMaskByte = 0;
	unsigned char ucCurBit = 0;
	unsigned char cInDataByte = 0;
	unsigned char *InData = NULL;
	unsigned char *ReadData = NULL;
	unsigned char *TmpReadData = NULL;
	if((InData = (unsigned char *) malloc((a_uiDataSize+7)/8+1)) == NULL)
		return ERR_OUT_OF_MEMORY;
	if((ReadData = (unsigned char *) malloc((a_uiDataSize+7)/8+1)) == NULL)
		return ERR_OUT_OF_MEMORY;
	for ( uiIndex = 0; uiIndex < a_uiDataSize; uiIndex++ ) { 
		if ( uiIndex % 8 == 0 ) {
			if ( g_usDataType & TDI_DATA ) {
				/*************************************************************
				*                                                            *
				* If the TDI_DATA flag is set, then grab the next byte from  *
				* the algo array and increment the TDI index.                *
				*                                                            *
				*************************************************************/
				ucTDIByte = GetByte( g_iTDIIndex++, 1 );
				InData[usBufferIndex++] = ucTDIByte;
			}
			else if ( g_usDataType & DTDI_DATA ){
				/*************************************************************
				*                                                            *
				* If TDI_DATA is not set, then DTDI_DATA must be set.  If    *
				* the compression counter exists, then the next TDI byte     *
				* must be 0xFF.  If it doesn't exist, then get next byte     *
				* from data file array.                                      *
				*                                                            *
				*************************************************************/
				if ( g_ucCompressCounter ) {
					g_ucCompressCounter--;
					ucTDIByte = ( unsigned char ) 0xFF;
					InData[usBufferIndex++] = ucTDIByte;
				}
				else {
					ucTDIByte = GetByte( g_iMovingDataIndex++, 0 );
					InData[usBufferIndex++] = ucTDIByte;

					/*************************************************************
					*                                                            *
					* If the frame is compressed and the byte is 0xFF, then the  *
					* next couple bytes must be read to determine how many       *
					* repetitions of 0xFF are there.  That value will be stored  *
					* in the variable g_ucCompressCounter.                       *
					*                                                            *
					*************************************************************/
					if ( ( g_usDataType & COMPRESS_FRAME ) && ( ucTDIByte == ( unsigned char ) 0xFF ) ) {
						g_ucCompressCounter = GetByte( g_iMovingDataIndex++, 0 );
						g_ucCompressCounter--;
					}
				}
			}
			else
			{
				InData[usBufferIndex++] = 0xFF;
			}
		}
	}	
	if(InData)
		free(InData);
	InData = NULL;	
	if ( g_usDataType & DTDI_DATA ){
		if ( GetByte( g_iMovingDataIndex++, 0 ) != I2C_END_FRAME ) {
			if (ReadData)
				free(ReadData);
			ReadData = NULL;
			return(ERR_DATA_FILE_ERROR);
		}
		if ( g_usDataType & COMPRESS ) {
			if ( g_usDataType & DTDO_DATA ) {
				g_usDataType &= ~( COMPRESS_FRAME );
				if ( GetByte( g_iMovingDataIndex++, 0 ) ) {
					g_usDataType |= COMPRESS_FRAME;
				}
			}
		}
	}
	usBufferIndex = 0;
	if( a_uiDataSize == 32)
	{
		if(ReadBytesAndSendNACK(a_uiDataSize, ReadData, 1))
		{
			if (ReadData)
				free(ReadData);
			ReadData = NULL;
			return(-1);
		}
	}
	else
	{
		if(ReadBytesAndSendNACK(a_uiDataSize, ReadData, 0))
		{
			if (ReadData)
				free(ReadData);
			ReadData = NULL;
			return(-1);
		}
	}
#ifdef I2C_DEBUG
		printf("\nEXPECTED TDO (" );
#endif
	
	usBufferIndex = 0;
	for ( uiIndex = 0; uiIndex < a_uiDataSize; uiIndex++ ) { 
		if ( uiIndex % 8 == 0 ) {
			if ( g_usDataType & TDO_DATA ) {
				/*************************************************************
				*                                                            *
				* If the TDO_DATA flag is set, then grab the next byte from  *
				* the algo array and increment the TDO index.                *
				*                                                            *
				*************************************************************/
				ucTDOByte = GetByte( g_iTDOIndex++, 1 );
#ifdef I2C_DEBUG
				printf("%.2X", ucTDOByte );
#endif
			}
			else {
				/*************************************************************
				*                                                            *
				* If TDO_DATA is not set, then DTDO_DATA must be set.  If    *
				* the compression counter exists, then the next TDO byte     *
				* must be 0xFF.  If it doesn't exist, then get next byte     *
				* from data file array.                                      *
				*                                                            *
				*************************************************************/
				if ( g_ucCompressCounter ) {
					g_ucCompressCounter--;
					ucTDOByte = ( unsigned char ) 0xFF;
				}
				else {
					ucTDOByte = GetByte( g_iMovingDataIndex++, 0 );
#ifdef I2C_DEBUG
					printf( "%.2X", ucTDOByte );
#endif

					/*************************************************************
					*                                                            *
					* If the frame is compressed and the byte is 0xFF, then the  *
					* next couple bytes must be read to determine how many       *
					* repetitions of 0xFF are there.  That value will be stored  *
					* in the variable g_ucCompressCounter.                       *
					*                                                            *
					*************************************************************/
					if ( ( g_usDataType & COMPRESS_FRAME ) && ( ucTDOByte == ( unsigned char ) 0xFF ) ) {
						g_ucCompressCounter = GetByte( g_iMovingDataIndex++, 0 );
						g_ucCompressCounter--;
					}
				}
			}

			if ( g_usDataType & MASK_DATA ) {
				ucMaskByte = GetByte( g_iMASKIndex++, 1 );
			}
			else { 
				ucMaskByte = ( unsigned char ) 0xFF;
			}
			cInDataByte = ReadData[usBufferIndex++];
		}
		ucCurBit = (unsigned char)(((cInDataByte << uiIndex%8) & 0x80) ? 0x01 : 0x00);	
		if ( ( ( ( ucMaskByte << uiIndex % 8 ) & 0x80 ) ? 0x01 : 0x00 ) ) {	
			if ( ucCurBit != ( unsigned char ) ( ( ( ucTDOByte << uiIndex % 8 ) & 0x80 ) ? 0x01 : 0x00 ) ) {
				usErrorCount++;  
			}
		}
	}
#ifdef I2C_DEBUG
		printf(  ");\n" );
#endif
#ifdef I2C_DEBUG
		printf("ACTUAL TDO (" );

		for ( int usDataSizeIndex = 0; usDataSizeIndex < (unsigned short)( ( a_uiDataSize + 7 ) / 8 ) ; usDataSizeIndex++ ) {
			cInDataByte = ReadData[ usDataSizeIndex ];
			printf( "%.2X", cInDataByte );
			if ( usDataSizeIndex % 40 == 39 ) {
				printf( "\n\t\t" );
			}
		}
		printf( ");\n" );		
#endif //I2C_DEBUG

	if (ReadData)
		free(ReadData);
	ReadData = NULL;
	if ( usErrorCount > 0 ) {
		return -1;
	}
	return 0;
}
/*************************************************************
*                                                            *
* ISPVMSEND                                                  *
*                                                            *
* INPUT:                                                     *
*     a_uiDataSize: this argument is the size of the         *
*     command.                                               *
*                                                            *
* RETURN:                                                    *
*     None.                                                  *
*                                                            *
* DESCRIPTION:                                               *
*     This function sends a data stream to the device.       *
*                                                            *
*************************************************************/

short int ispVMSend( unsigned int a_uiDataSize )
{
	unsigned int iIndex;
	unsigned int usBufferIndex = 0;
	unsigned char ucCurByte = 0;
	unsigned char *g_pucInData = NULL;
	if((g_pucInData = (unsigned char *) malloc((a_uiDataSize+7)/8+1)) == NULL)
		return -1;
	/*************************************************************
	*                                                            *
	* Begin processing the data to the device.                   *
	*                                                            *
	*************************************************************/
	for ( iIndex = 0; iIndex < a_uiDataSize; iIndex++ ) { 
		if ( iIndex % 8 == 0 ) { 
			if ( g_usDataType & TDI_DATA ) {
				/*************************************************************
				*                                                            *
				* If the TDI_DATA flag is set, then grab the next byte from  *
				* the algo array and increment the TDI index.                *
				*                                                            *
				*************************************************************/
				ucCurByte = GetByte( g_iTDIIndex++, 1 );
			}
			else {
				/*************************************************************
				*                                                            *
				* If TDI_DATA flag is not set, then DTDI_DATA flag must have *
				* already been set.  If the compression counter exists, then *
				* the next TDI byte must be 0xFF.  If it doesn't exist, then *
				* get next byte from data file array.                        *
				*                                                            *
				*************************************************************/
				if ( g_ucCompressCounter ) {
					g_ucCompressCounter--;
					ucCurByte = ( unsigned char ) 0xFF;
				}
				else {
					ucCurByte = GetByte( g_iMovingDataIndex++, 0 );

					/*************************************************************
					*                                                            *
					* If the frame is compressed and the byte is 0xFF, then the  *
					* next couple bytes must be read to determine how many       *
					* repetitions of 0xFF are there.  That value will be stored  *
					* in the variable g_ucCompressCounter.                       *
					*                                                            *
					*************************************************************/

					if ( ( g_usDataType & COMPRESS_FRAME ) && ( ucCurByte == ( unsigned char ) 0xFF ) ) {
						g_ucCompressCounter = GetByte( g_iMovingDataIndex++, 0 );
						g_ucCompressCounter--;
					}
				}
			}
			g_pucInData[usBufferIndex++] = ucCurByte;
		}
	}			
	if(SendBytesAndCheckACK(a_uiDataSize, g_pucInData))
	{
		printf("Failed to get ACK when send byte.\n");
		if(g_pucInData)
			free(g_pucInData);
		g_pucInData = NULL;
		return ERR_VERIFY_ACK_FAIL;
	}
#ifdef I2C_DEBUG
		printf("TDI (" );
		unsigned char cInDataByte = 0;
		unsigned char cDataByte = 0;
		for ( int usDataSizeIndex = (unsigned short)( ( a_uiDataSize + 7 ) / 8 ); usDataSizeIndex > 0 ; usDataSizeIndex-- ) {
			cInDataByte = g_pucInData[ usDataSizeIndex - 1 ];
			cDataByte = 0x00;

			/****************************************************************************
			*
			* Flip cMaskByte and store it in cDataByte.
			*
			*****************************************************************************/

			for ( usBufferIndex = 0; usBufferIndex < 8; usBufferIndex++ ) {
				cDataByte <<= 1;
				if ( cInDataByte & 0x01 ) {
					cDataByte |= 0x01;
				}
				cInDataByte >>= 1;
			}
			printf( "%.2X", cDataByte );
			if ( ( ( ( a_uiDataSize + 7 ) / 8 ) - usDataSizeIndex ) % 40 == 39 ) {
				printf( "\n\t\t" );
			}
		}
		printf( ");\n" );		
#endif //I2C_DEBUG
	if(g_pucInData)
		free(g_pucInData);
	g_pucInData = NULL;
	return 0;
}
void ispVMComment()
{
	unsigned char currentByte  = 0;	
	do{
		currentByte = GetByte( g_iMovingAlgoIndex++, 1 );
		if(currentByte == I2C_ENDCOMMENT)
			break;
		printf( "%c", currentByte);
	}while(currentByte != I2C_ENDCOMMENT);	
	printf( "\n" );	
}
/***************************************************************
*
* ispVMLoop
*
* Perform the function call upon by the REPEAT opcode.
* Memory is to be allocated to store the entire loop from REPEAT to ENDLOOP.
* After the loop is stored then execution begin. The REPEATLOOP flag is set
* on the g_usFlowControl register to indicate the repeat loop is in session
* and therefore fetch opcode from the memory instead of from the file.
*
***************************************************************/

short int ispVMLoop(unsigned short a_usLoopCount, int type)
{
	short int siRetCode = 0;
	unsigned short usContinue = 1;
	unsigned char ucOpcode = 0;
	unsigned int uiDataSize = 0;
	int intDelay = 0;
	
	/*************************************************************
	*                                                            *
	* Set the repeat index to the first byte in the repeat loop. *
	*                                                            *
	*************************************************************/

	g_iLoopMovingIndex = g_iMovingAlgoIndex;
	g_iLoopDataMovingIndex = g_iMovingDataIndex;

	for ( g_iLoopIndex = 0 ; g_iLoopIndex < g_usLCOUNTSize; g_iLoopIndex++ ) 
	{
		usContinue	= 1;
		/*************************************************************
		*                                                            *
		* Initialize the current algorithm index to the beginning of *
		* the repeat index before each repeat loop.                  *
		*                                                            *
		*************************************************************/

		g_iMovingAlgoIndex = g_iLoopMovingIndex;
		g_iMovingDataIndex = g_iLoopDataMovingIndex;

		while ( usContinue ) 
		{
			ucOpcode = GetByte( g_iMovingAlgoIndex++, 1 );
			switch ( ucOpcode ) 
			{
				case I2C_STARTTRAN:
#ifdef I2C_DEBUG
			printf("Start Condition\n");
#endif /* I2C_DEBUG */
					siRetCode = SetI2CStartCondition(type);
					break;
				case I2C_RESTARTTRAN:
#ifdef I2C_DEBUG
			printf("ReStart Condition\n");
#endif /* I2C_DEBUG */
					siRetCode = SetI2CReStartCondition();
					break;
				case I2C_ENDTRAN:
#ifdef I2C_DEBUG
			printf("ReStart Condition\n");
#endif /* I2C_DEBUG */
					siRetCode = SetI2CStopCondition();
					break;
				case I2C_TRANSOUT:
					/*************************************************************
					*                                                            *
					* Execute SIR/SDR command.                                   *
					*                                                            *
					*************************************************************/
					siRetCode = ispVMShift( ucOpcode );
					break;
				case I2C_TRANSIN:
					/*************************************************************
					*                                                            *
					* Execute SIR/SDR command.                                   *
					*                                                            *
					*************************************************************/
					siRetCode = ispVMShift( ucOpcode );
					if ( siRetCode >= 0 ) {
						/****************************************************************************
						*
						* Break if intelligent programming is successful.
						*
						*****************************************************************************/

						return ( siRetCode );
					}
					else
						usContinue = 0;
					break;		
				case I2C_WAIT:
					/*************************************************************
					*                                                            *
					* Issue delay in specified time.                             *
					*                                                            *
					*************************************************************/
					intDelay = ispVMDataSize();
#ifdef I2C_DEBUG
			printf("Delay %d\n", intDelay);
#endif /* I2C_DEBUG */
					SetI2CDelay( intDelay );
					break;
				case I2C_COMMENT:
					ispVMComment();
					break;
			}
		}	
	}
	return ( siRetCode );
}
/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2011
* 
*
***************************************************************/


/**************************************************************
* 
* Revision History of i2c_main.c
* 
* 
* Support version 1.0
***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "opcode.h"
#include <utils/Log.h>
#define LOG_TAG "i2c_controller"

/***************************************************************
*
* Supported I2C versions.
*
***************************************************************/

const char * const g_szSupportedVersions[] = { "_I2C1.0", 0 };

/*************************************************************
*                                                            *
* EXTERNAL FUNCTIONS                                         *
*                                                            *
*************************************************************/
extern unsigned char GetByte( int a_iCurrentIndex, char a_cAlgo );
extern short ispProcessI2C(int type);
extern void EnableHardware(int type);
extern void DisableHardware();

/*************************************************************
*                                                            *
* GLOBAL VARIABLES                                           *
*                                                            *
*************************************************************/

unsigned char * g_pucAlgoArray = NULL;	/*** array to hold the algorithm ***/
unsigned char * g_pucDataArray = NULL;	/*** array to hold the data ***/
int g_iAlgoSize = 0;					/*** variable to hold the size of the algorithm array ***/
int g_iDataSize = 0;					/*** variable to hold the size of the data array ***/

/*************************************************************
*                                                            *
* EXTERNAL VARIABLES                                         *
*                                                            *
*************************************************************/

extern unsigned short g_usDataType;
extern int g_iMovingAlgoIndex;
extern int g_iMovingDataIndex;

/*************************************************************
*                                                            *
* ISPI2CNTRYPOINT                                            *
*                                                            *
* INPUT:                                                     *
*     a_pszAlgoFile: this is the name of the algorithm file. *
*                                                            *
*     a_pszDataFile: this is the name of the data file.      *
*     Note that this argument may be empty if the algorithm  *
*     does not require a data file.                          *
*                                                            *
* RETURN:                                                    *
*     The return value will be a negative number if an error *
*     occurred, or 0 if everything was successful            *
*                                                            *
* DESCRIPTION:                                               *
*     This function opens the file pointers to the algo and  *
*     data file.  It intializes global variables to their    *
*     default values and enters the processor.               *
*                                                            *
*************************************************************/

short int ispEntryPoint( const char * a_pszAlgoFile, const char * a_pszDataFile, int type )
{
	char szFileVersion[ 9 ] = { 0 };
	short int siRetCode     = 0;
	int iIndex              = 0;
	int cVersionIndex       = 0;
	FILE * pFile            = NULL;
	
	/*************************************************************
	*                                                            *
	* VARIABLES INITIALIZATION                                   *
	*                                                            *
	*************************************************************/

	g_pucAlgoArray     = NULL;	
	g_pucDataArray     = NULL;	
	g_iAlgoSize        = 0;
	g_iDataSize        = 0;
	g_usDataType       = 0;
	g_iMovingAlgoIndex = 0;
	g_iMovingDataIndex = 0;

	/*************************************************************
	*                                                            *
	* Open the algorithm file, get the size in bytes, allocate   *
	* memory, and read it in.                                    *
	*                                                            *
	*************************************************************/

	if ( ( pFile = fopen( a_pszAlgoFile, "rb" ) ) == NULL ) {
		return ERR_FIND_ALGO_FILE;
	}

	for ( g_iAlgoSize = 0; !feof( pFile ); g_iAlgoSize++ ) {
		getc( pFile );
	}
	g_iAlgoSize--;

	g_pucAlgoArray = ( unsigned char * ) malloc( g_iAlgoSize + 1 );
	if ( !g_pucAlgoArray ) {
		fclose( pFile );
		return ERR_OUT_OF_MEMORY;
	}

	rewind( pFile );
	for ( iIndex = 0; !feof( pFile ); ++iIndex ) {
		g_pucAlgoArray[ iIndex ] = (unsigned char) getc( pFile );
	}
	fclose( pFile );
	
	/*************************************************************
	*                                                            *
	* Open the data file, get the size in bytes, allocate        *
	* memory, and read it in.                                    *
	*                                                            *
	*************************************************************/

	if ( a_pszDataFile ) {
		if ( ( pFile = fopen( a_pszDataFile, "rb" ) ) == NULL ) {
			free( g_pucAlgoArray );
			return ERR_FIND_DATA_FILE;
		}

		for ( g_iDataSize = 0; !feof( pFile ); g_iDataSize++ ) {
			getc( pFile );
		}
		g_iDataSize--;

		g_pucDataArray = ( unsigned char * ) malloc( g_iDataSize + 1 );
		if ( !g_pucDataArray ) {
			free( g_pucAlgoArray );
			fclose( pFile );
			return ERR_OUT_OF_MEMORY;
		}

		rewind( pFile );
		for ( iIndex = 0; !feof( pFile ); ++iIndex ) {
			g_pucDataArray[ iIndex ] = (unsigned char) getc( pFile );
		}
		fclose( pFile );

		if ( GetByte( g_iMovingDataIndex++, 0 ) ) {
			g_usDataType |= COMPRESS;
		}
	}

	/***************************************************************
	*
	* Read and store the version of the VME file.
	*
	***************************************************************/

	for ( iIndex = 0; iIndex < strlen(g_szSupportedVersions[0]); iIndex++ ) {
		szFileVersion[ iIndex ] = GetByte( g_iMovingAlgoIndex++, 1 );
	}

	/***************************************************************
	*
	* Compare the VME file version against the supported version.
	*
	***************************************************************/

	for ( cVersionIndex = 0; g_szSupportedVersions[ cVersionIndex ] != 0; cVersionIndex++ ) {
		for ( iIndex = 0; iIndex < strlen(g_szSupportedVersions[cVersionIndex]); iIndex++ ) {
			if ( szFileVersion[ iIndex ] != g_szSupportedVersions[ cVersionIndex ][ iIndex ] ) {
				siRetCode = ERR_WRONG_VERSION;
				break;
			}	
			siRetCode = 0;
		}

		if ( siRetCode == 0 ) {

			/***************************************************************
			*
			* Found matching version, break.
			*
			***************************************************************/

			break;
		}
	}

	if ( siRetCode < 0 ) {

		/***************************************************************
		*
		* VME file version failed to match the supported versions.
		*
		***************************************************************/

		free( g_pucAlgoArray );
		if ( g_pucDataArray ) {
			free( g_pucDataArray );
		}
		g_pucAlgoArray = NULL;
		g_pucDataArray = NULL;
		return ERR_WRONG_VERSION;
	}
                   
	/*************************************************************
	*                                                            *
	* Start the hardware.                                        *
	*                                                            *
	*************************************************************/

    EnableHardware(type);
	
	/*************************************************************
	*                                                            *
	* Begin processing algorithm and data file.                  *
	*                                                            *
	*************************************************************/

	siRetCode = ispProcessI2C(type);

	/*************************************************************
	*                                                            *
	* Stop the hardware.                                         *
	*                                                            *
	*************************************************************/

    DisableHardware();

	/*************************************************************
	*                                                            *
	* Free dynamic memory and return value.                      *
	*                                                            *
	*************************************************************/

	free( g_pucAlgoArray );
	if ( g_pucDataArray ) {
		free( g_pucDataArray );
	}
	g_pucAlgoArray = NULL;
	g_pucDataArray = NULL;

    return ( siRetCode );
}

/*************************************************************
*                                                            *
* ERROR_HANDLER                                              *
*                                                            *
* INPUT:                                                     *
*     a_siRetCode: this is the error code reported by the    *
*     processor.                                             *
*                                                            *
*     a_pszMessage: this will store the return message.      *
*                                                            *
* RETURN:                                                    *
*     None.                                                  *
*                                                            *
* DESCRIPTION:                                               *
*     This function assigns an error message based on the    *
*     error reported by the processor.  The program should   *
*     display the error message prior to exiting.            *
*                                                            *
*************************************************************/

void error_handler( short int a_siRetCode, char * a_pszMessage )
{
	switch( a_siRetCode ) {
	case ERR_VERIFY_FAIL:
		strcpy( a_pszMessage, "VERIFY FAIL" );
		break;
	case ERR_FIND_ALGO_FILE:
		strcpy( a_pszMessage, "CANNOT FIND ALGO FILE" );
		break;
	case ERR_FIND_DATA_FILE:
		strcpy( a_pszMessage, "CANNOT FIND DATA FILE" );
		break;
	case ERR_WRONG_VERSION:
		strcpy( a_pszMessage, "WRONG FILE TYPE/VERSION" );
		break;
	case ERR_ALGO_FILE_ERROR:
		strcpy( a_pszMessage, "ALGO FILE ERROR" );
		break;
	case ERR_DATA_FILE_ERROR:
		strcpy( a_pszMessage, "DATA FILE ERROR" );
		break;
	case ERR_OUT_OF_MEMORY:
		strcpy( a_pszMessage, "OUT OF MEMORY" );
		break;
	default:
		strcpy( a_pszMessage, "UNKNOWN ERROR" );
		break;
	}
} 

/*************************************************************
*                                                            *
* MAIN                                                       *
*                                                            *
*************************************************************/

short int load(char * alg_file_path, char * data_file_path, int type)
{
	/*************************************************************
	*                                                            *
	* LOCAL VARIABLES:                                           *
	*                                                            *
	*************************************************************/
	unsigned short iCommandLineIndex	= 0;
	char szCommandLineArg[ 1024 ]       = { 0 };
	char szExtension[ 5 ]				= { 0 };
	char *szCommandLineArgPtr			= 0;
	short int siRetCode                 = 0; 
	char szErrorMessage[ 50 ]           = { 0 };
	
	printf( "        Lattice Semiconductor Corp.\n" );
	printf( "    ispI2C(tm) 1.0 Copyright 2011.\n\n" );

//	for ( iCommandLineIndex = 1; iCommandLineIndex < argc; iCommandLineIndex++ ) {
//		strcpy( szCommandLineArg, argv[ iCommandLineIndex ] );
//		
//		if( strchr(szCommandLineArg, '\"') != strrchr(szCommandLineArg, '\"') ){
//			szCommandLineArgPtr = strchr(szCommandLineArg, '\"');
//			*strrchr(szCommandLineArg, '\"') = 0;
//		}
//		else{
//			szCommandLineArgPtr = strchr(szCommandLineArg, '\"');
//			if(szCommandLineArgPtr)
//				*szCommandLineArgPtr = 0;
//			else
//				szCommandLineArgPtr = szCommandLineArg;
//		}
//		
//		strcpy( szExtension, &szCommandLineArgPtr[ strlen( szCommandLineArg ) - 4 ] );
//		strlwr( szExtension );
//		if ( stricmp( szExtension, ".iea" ) && stricmp( szExtension, ".ied" )  ) 
//		{
//			printf( "Error: I2C files must end with the extension *.iea or *.ied\n\n" );
//			exit( -1 );
//		}
//	}
	siRetCode = 0;
	/*************************************************************
	*                                                            *
	* Pass in the command line arguments to the entry point.     *
	*                                                            *
	*************************************************************/

	siRetCode = ispEntryPoint( alg_file_path, data_file_path, type );
	/*************************************************************
	*                                                            *
	* Check the return code and report appropriate message       *
	*                                                            *
	*************************************************************/
	if ( siRetCode < 0 ) {
		error_handler( siRetCode, szErrorMessage );
        printf( "\nProcessing failure: %s\n\n", szErrorMessage );
        printf( "+=======+\n" );
        printf( "| FAIL! |\n" );
        printf( "+=======+\n\n" );
	} 
	else {
		printf( "\n+=======+\n" );
        printf( "| PASS! |\n" );
        printf( "+=======+\n\n" );
	}
//	exit( siRetCode );
	return siRetCode;
} 

/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2020, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Parkway, Norcross,                    **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 *
 * PDKSDR.c
 * SDR related Functions.
 *
 *  Author: Winston <winstonv@amiindia.co.in>
 ******************************************************************/

#define ENABLE_DEBUG_MACROS 0

#include "PDKSDR.h"
#include "NVRAPI.h"
#include "IPMIConf.h"


/**
*@fn PDKInitNVRSDR 
*@brief This function is invoked to load SDR records from NVRAM to RAM
*@param pData - Pointer to buffer where SDR records have to be initialized
*@param Size - Size of the SDR repository
*@return Returns 0 on success
*/
int PDKInitNVRSDR(INT8U* pData,INT32U Size, int BMCInst)
{
    char SDRFile[64];
    int len;
    SDR_FILE(BMCInst,&PlatformID[0],SDRFile);
    //This API to read NVRAM can be replaced by OEM's if they need to use EEPROM
    len = API_ReadNVR(SDRFile,0,Size,pData);
    return len;
}

/**
*@fn WriteSDR 
*@brief This function is invoked to wirte SDR records to NVRAM
*@param pData - Pointer to buffer from where SDR records have to be written in NVRAM
*@param Size - Size of the SDR records to be written
*@return Returns 0 on success
*/
int PDKWriteSDR(INT8U* pData,INT32U Offset,INT16U Size, int BMCInst)
{
    char SDRFile[64];
    int len;
    SDR_FILE(BMCInst,&PlatformID[0],SDRFile);
     //This API to write NVRAM can be replaced by OEM's if they need to use EEPROM
    len = API_WriteNVR(SDRFile,Offset,Size,pData);
    return len;
}



 


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
 * PDKSEL.c
 * SEL related Functions.
 *
 *  Author: Winston <winstonv@amiindia.co.in>
 ******************************************************************/

#define ENABLE_DEBUG_MACROS 0

#include "PDKSEL.h"
#include "NVRAPI.h"

/**
*@fn PDKInitNVRSEL 
*@brief This function is invoked to load SEL entries from NVRAM to RAM
*@param pData - Pointer to buffer where SEL records have to be initialized
*@param Size - Size of the SEL repository
*@return Returns length on success; return -1 on failure
*/
int PDKInitNVRSEL(INT8U* pData,INT32U Size, int BMCInst)
{
    char SELFile[64];
    int len;
    SEL_FILE(BMCInst,SELFile);
    //This API to read NVRAM can be replaced by OEM's if they need to use EEPROM
    len = API_ReadNVR(SELFile,0,Size,pData);
    return len;
}

/**
*@fn WriteSEL 
*@brief This function is invoked to Write SEL entries to NVRAM
*@param pData - Pointer to buffer from where SEL records have to be written in NVRAM 
*@param Size - Size of the SEL entries to be written
*@return Returns 0 on success
*/
int PDKWriteSEL(INT8U* pData,INT32U Offset,INT32U Size,int BMCInst)
{
    char SELFile[64];
    int len;
    SEL_FILE(BMCInst,SELFile);
    //This API to write NVRAM can be replaced by OEM's if they need to use EEPROM
    len = API_WriteNVR(SELFile,Offset,Size,pData);
    return len;
}






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
 * PDKFRU.c
 * fru functions.
 *
 *  Author: Rama Bisa <ramab@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "FRU.h"
#include "IPMI_FRU.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "Debug.h"
#include "NVRAccess.h"
#include "Util.h"
#include "NVRAPI.h"
#include "PDKEEPROM.h"
#include "PDKFRU.h"
#include <stdarg.h>
#include "hal_api.h"
#include "IPMIConf.h"
#include "featuredef.h"
#include "OEMFRU.h"

/*** Local Definitions ***/
//#define     DEV_ACCESS_MODE_IN_BYTES    0x00
#define     IPMI_FRU_FORMAT_VERSION     0x01
#define     CC_FRU_DATA_UNAVAILABLE     0x81

//#define FRU_FILE_SIZE  256
//#define FRU_FILE         "/conf/BMC1/FRU.bin"
#define FRU_FILE_NAME_LEN   20
#define FRU_FILE(Instance,filename) \
snprintf(filename,FRU_FILE_NAME_LEN,"%s%d/%s",NV_DIR_PATH,Instance,"FRU.bin")


FRUAccess_T g_FruTbl [] = 
{
    {FRU_ID_COME,   FRU_EEPROM_ADDR_COME,     1},
    {FRU_ID_BMC,    FRU_EEPROM_ADDR_BMC,      1},
    {FRU_ID_SYS,    FRU_EEPROM_ADDR_SYS,      1},
    {FRU_ID_FIOB,   FRU_EEPROM_ADDR_FIOB,     1},
    {FRU_ID_FCB,    FRU_EEPROM_ADDR_FCB,      1},
    {FRU_ID_FAN1,   FRU_EEPROM_ADDR_FAN1,     1},
    {FRU_ID_FAN2,   FRU_EEPROM_ADDR_FAN2,     1},
    {FRU_ID_FAN3,   FRU_EEPROM_ADDR_FAN3,     1},
    {FRU_ID_FAN4,   FRU_EEPROM_ADDR_FAN4,     1},
    {FRU_ID_FAN5,   FRU_EEPROM_ADDR_FAN5,     1},
    {FRU_ID_PSUB,   FRU_EEPROM_ADDR_PSUB,     1},
    {FRU_ID_PSU1,   FRU_EEPROM_ADDR_PSU1,     1},
    {FRU_ID_PSU2,   FRU_EEPROM_ADDR_PSU2,     1},
    {FRU_ID_PSU3,   FRU_EEPROM_ADDR_PSU3,     1},
    {FRU_ID_PSU4,   FRU_EEPROM_ADDR_PSU4,     1},
};


/*** Module Variables ***/
//static INT8U		m_total_frus = 1;
//extern FRUInfo_T    *m_FRUInfo [MAX_PDK_FRU_SUPPORTED];
       /*{{0 , 0 , FRU_TYPE_NVR , FRU_FILE_SIZE , DEV_ACCESS_MODE_IN_BYTES , FRU_FILE , 0 , 0 , 0 , 0}};*/
#if 0
/*-----------------------------------------------------
 * PDK_RegisterFRU
 *----------------------------------------------------*/
int
PDK_RegisterFRU (FRUInfo_T *pFRUInfo, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /* PDK Register FRU  */
    if (pBMCInfo->FRUConfig.total_frus >= MAX_PDK_FRU_SUPPORTED)
    {
        /* No room for FRU initialization */
        IPMI_WARNING ("No slot available for this registration \n");
        return -1;
    }

    /* Register FRU */
    pBMCInfo->FRUConfig.m_FRUInfo [pBMCInfo->FRUConfig.total_frus] = pFRUInfo;
    pBMCInfo->FRUConfig.total_frus++;

    return 0;
}
#endif

int PDK_RegisterAllFRUs(int BMCInst)
{
	if(0)
	{
	    BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
	}
#if 0
	printf("PDK_RegisterAllFRUs\n");
	FRUConfig	fruCfg;
	FRUInfo_T	FRUInfo;
	int handle = 0;

	/* If there are more FRUs then this process needs to be repeated for all.
	   The following code assumes that there is only one EEPROM FRU */

	handle = hal_get_device_handle (0x0, BMCInst);
	/* Get all the device handles with the same apptype */
	if(handle != INVALID_HANDLE)
	{
		/* Get Device Map */
		if (-1 != hal_device_get_properties (handle,(u8*)&fruCfg, BMCInst))
		{
	//		printf("frCfg.BusNo = %d\n",fruCfg.BusNo);
	//		printf("frCfg.SlaveAddr = %d\n",fruCfg.SlaveAddr);
	//		printf("frCfg.FRU_ID = %d\n",fruCfg.FRU_ID);
	//		printf("frCfg.FRU_Size = %d\n",fruCfg.FRU_Size);
	//		printf("frCfg.FRU_Offset = %d\n",fruCfg.FRU_Offset);
	//		printf("frCfg.PageSize = %d\n",fruCfg.PageSize);

			/* Register System FRU */
			FRUInfo.DeviceID                = fruCfg.FRU_ID;          // FRU Device ID
			FRUInfo.Type                    = FRU_TYPE_EEPROM;             // Type is EEPROM

			FRUInfo.Size                    = (fruCfg.FRU_Size) * 128;
			FRUInfo.AccessType              = 0;
			memset(FRUInfo.NVRFile, 0, sizeof(FRUInfo.NVRFile));
			FRUInfo.BusNumber               = fruCfg.BusNo;
			FRUInfo.SlaveAddr               = fruCfg.SlaveAddr >> 1;

			/* Please check what Device ID you assigned in PMCP to this EEPROM FRU.
			   Adjust the device type accordingly here. The following code means that
			   Device ID 0 was assigned to SEEPROM type FRU in PMCP and EEPROM IPMI type
			   code for this device Id is 0x0F which is actually 24C64 */

			if (FRUInfo.DeviceID == 0)
			{
					FRUInfo.DeviceType      = 0x0F;
			}
			else
			{
					FRUInfo.DeviceType      = 0x09;
			}
			FRUInfo.Offset                  = 0x00;
		}
		else
			printf("Invalid properties\n");
	}
	else
		printf("Invalid handle\n");

	/* Registration success */
	return  PDK_RegisterFRU (&FRUInfo, BMCInst);
#endif
return 0;
}

/*-----------------------------------------------------
 * GetFRUInfo
 *----------------------------------------------------*/
static FRUInfo_T* GetFRUInfo (INT8U DeviceID, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int i;

    for (i = 0; i < pBMCInfo->FRUConfig.total_frus; i++)
    {
        if (DeviceID == pBMCInfo->FRUConfig.m_FRUInfo [i]->DeviceID)
        {
            return pBMCInfo->FRUConfig.m_FRUInfo [i];
        }
    }
    return NULL;
}

static int ReadFRUDevice (FRUInfo_T* pFRUInfo, INT16U Offset, INT8U Len, INT8U* pData, int BMCInst)
{

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    switch (pFRUInfo->Type)
    {
        case FRU_TYPE_NVR:
        {
            return  API_ReadNVR (pFRUInfo->NVRFile, Offset, Len, pData);
            break;
        }

        case FRU_TYPE_EEPROM:
            return  READ_EEPROM (pFRUInfo->DeviceType, pFRUInfo->BusNumber, pFRUInfo->SlaveAddr, pData,
                                                 pFRUInfo->Offset + Offset, Len);
            break;
        
    default:
        IPMI_WARNING ("PDKFRU.c : FRU Type not supported\n");
        return 0;
    }

    return 0;
}

static int WriteFRUDevice (FRUInfo_T* pFRUInfo, INT16U Offset, INT8U Len, INT8U* pData, int BMCInst)
{

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    switch (pFRUInfo->Type)
    {
        case FRU_TYPE_NVR:
        {
            return  API_WriteNVR (pFRUInfo->NVRFile, Offset, Len, pData);
            break;
        }

        case FRU_TYPE_EEPROM:
            return  WRITE_EEPROM (pFRUInfo->DeviceType, pFRUInfo->BusNumber, pFRUInfo->SlaveAddr, pData,
                                                    pFRUInfo->Offset + Offset, Len);
            break;

        default:
            IPMI_WARNING ("PDKFRU.c : FRU Type not supported\n");
            return 0;
    }

    return 0;
}


/*-----------------------------------------------------
 * GetFRUAreaInfo
 *----------------------------------------------------*/
int
PDK_GetFRUAreaInfo (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, int BMCInst)
{
    FRUInfo_T*                  pFRUInfo;
    FRUInventoryAreaInfoReq_T*  pFRUAreaInfoReq = (FRUInventoryAreaInfoReq_T*) pReq;
    FRUInventoryAreaInfoRes_T*  pFRUAreaInfoRes = (FRUInventoryAreaInfoRes_T*) pRes;

    /* Get the FRU information for the requested device ID */
    pFRUInfo = GetFRUInfo (pFRUAreaInfoReq->FRUDeviceID,BMCInst);

    if(0)
    {
        ReqLen=ReqLen;  /*  -Wextra, fix for unused parameter  */
    }
    /* If requested device Id not found */
    if (NULL == pFRUInfo)
    {
        pFRUAreaInfoRes->CompletionCode = CC_FRU_REC_NOT_PRESENT;
        return sizeof(INT8U);
    }

    pFRUAreaInfoRes->CompletionCode     = CC_NORMAL;
    pFRUAreaInfoRes->Size               = htoipmi_u16 (pFRUInfo->Size);
    pFRUAreaInfoRes->DeviceAccessMode   = pFRUInfo->AccessType;

    return sizeof(FRUInventoryAreaInfoRes_T);
}

/*-----------------------------------------------------
 * Read FRU Data
 *----------------------------------------------------*/
int
PDK_ReadFRUData (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, int BMCInst)
{

    INT16U          Offset;
    FRUReadReq_T*   pFRUReadReq = (FRUReadReq_T*) pReq;
    FRUReadRes_T*   pFRUReadRes = (FRUReadRes_T*) pRes;
    FRUInfo_T*      pFRUInfo;
    int retval = 0;

    /* Get the FRU information for the requested device ID */
    pFRUInfo = GetFRUInfo (pFRUReadReq->FRUDeviceID,BMCInst);

    if(0)
    {
        ReqLen=ReqLen;  /*  -Wextra, fix for unused parameter  */
    }
    /* If requested device Id not found */
    if (NULL == pFRUInfo)
    {
        pFRUReadRes->CompletionCode = CC_FRU_REC_NOT_PRESENT;
        return sizeof(INT8U);
    }

    /* Get the offset to read */
    Offset = htoipmi_u16 (pFRUReadReq->Offset);;

    /* check if the offset is valid offset */
    if (Offset >= pFRUInfo->Size)
    {
        pFRUReadRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Is CountToRead too big? */
    if (pFRUReadReq->CountToRead > (pFRUInfo->Size - Offset))
    {


    pFRUReadRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
    return sizeof(INT8U);
        //pFRUReadReq->CountToRead = pFRUInfo->Size - Offset;
    }

    /* Is CountToRead zero */
    if ( 0 == pFRUReadReq->CountToRead)
    {
        pFRUReadRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    pFRUReadRes->CompletionCode  =  CC_NORMAL;

    /* Read the FRU date from the FRU device */
    retval = ReadFRUDevice (pFRUInfo, Offset,
                                    pFRUReadReq->CountToRead,
                                    (INT8U*)(pFRUReadRes + 1), BMCInst);

    if (retval >= 0)
    {
        pFRUReadRes->CountReturned = retval;
    /*Updating Response Length.*/
	return (pFRUReadRes->CountReturned + sizeof(FRUReadRes_T));
    }
    else
    {
        pFRUReadRes->CompletionCode = CC_FRU_DATA_UNAVAILABLE;
        return sizeof(INT8U);
    }
}


/*-----------------------------------------------------
 * Write FRU Data
 *----------------------------------------------------*/
int
PDK_WriteFRUData (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, int BMCInst)
{
    INT16U          Offset,Length;
    FRUWriteReq_T   *pFRUWriteReq = (FRUWriteReq_T*) pReq;
    FRUWriteRes_T   *pFRUWriteRes = (FRUWriteRes_T*) pRes;
    FRUInfo_T       *pFRUInfo = NULL;
    int 			retval = 0;

    /* Get the FRU information for the requested device ID */
    pFRUInfo = GetFRUInfo (pFRUWriteReq->FRUDeviceID,BMCInst);

    /* If requested device Id not found */
    if (NULL == pFRUInfo)
    {
        pFRUWriteRes->CompletionCode = CC_FRU_REC_NOT_PRESENT;
        return sizeof(INT8U);
    }

    /* Get the offset to write & number of bytes to write */
    Offset  = htoipmi_u16 (pFRUWriteReq->Offset);
    Length  = ReqLen - sizeof (FRUWriteReq_T);

    /* check if the offset is valid offset */
    if (Offset >= pFRUInfo->Size)
    {
        pFRUWriteRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Is Length too big? */
    if (Length > (pFRUInfo->Size - Offset))
    {


    pFRUWriteRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
    return sizeof(INT8U);
      //  Length = pFRUInfo->Size - Offset;
    }

    pFRUWriteRes->CompletionCode = CC_NORMAL;

    /* Wrtie the date to FRU device */
    retval = WriteFRUDevice (pFRUInfo, Offset, Length,
                                                (INT8U*)(pFRUWriteReq + 1),BMCInst);
    if (retval != -1)
    {
    	pFRUWriteRes->CountWritten  = (INT8U) retval;
    }
    else
    {
        pFRUWriteRes->CompletionCode = CC_FRU_DATA_UNAVAILABLE;
        return sizeof(INT8U);
    }


    return sizeof(FRUWriteRes_T);
}


/*-----------------------------------------------------
 * InitMultiRecord
 *-----------------------------------------------------*/
int
InitMultiRecord (INT8U FRUId, INT8U NumMultiRec, int MultiRecId, ...)
{
    FRUInfo_T			*pFRUInfo;
    FRUInfo_T			*pMultiRecFRUInfo;
    FRUCommonHeader_T	CommonHdr;
    MultiRecordHeader_T	MultiRecordHdr;
    INT16U				MultiRecOffset;
    va_list				pArgs;

int BMCInst = 1;
    va_start (pArgs, MultiRecId);

    pFRUInfo = GetFRUInfo (FRUId,BMCInst);
    if (pFRUInfo == NULL)
    {
        IPMI_WARNING ("PDKFRU.c : FRU device id %d not found\n", FRUId);
        va_end (pArgs);
        return -1;
    }

    /* Read the Common header */
    ReadFRUDevice (pFRUInfo, 0, sizeof (FRUCommonHeader_T), (INT8U*)&CommonHdr,BMCInst);

    if (CommonHdr.CommonHeaderFormatVersion != 0x01)
    {
        IPMI_WARNING ("PDKFRU.c : InitMultiRecord Invalid FRU version for Id %d\n", FRUId);
        va_end (pArgs);
        return 0;
    }

    MultiRecOffset = (8 * CommonHdr.MultiRecordAreaStartOffset);

    /* Check if multi record is present */
    if (0 == MultiRecOffset)
    {
        IPMI_WARNING ("PDKFRU.c : Multi record FRU not found for FRU Id %d\n", FRUId);
        va_end (pArgs);
        return 0;
    }

    do
    {
        /* Read the multi Record */
        ReadFRUDevice (pFRUInfo, MultiRecOffset, sizeof (MultiRecordHeader_T) - sizeof (INT8U*), (INT8U*)&MultiRecordHdr,BMCInst);

        /* Get the FRU information for this multi record */
        pMultiRecFRUInfo = GetFRUInfo (MultiRecId,BMCInst);
        if (pMultiRecFRUInfo == NULL)
        {
            va_end (pArgs);
            IPMI_WARNING ("PDKFRU.c : FRU device id %d not found\n", MultiRecId);
            return -1;
        }

        /* Set the FRU device offset to this multi record */
        pMultiRecFRUInfo->Offset += MultiRecOffset + (sizeof (MultiRecordHeader_T) - sizeof (INT8U*)) + 18;
        pMultiRecFRUInfo->Size    = MultiRecordHdr.RecordLength;

        IPMI_DBG_PRINT_3 ("Start offset and size for Record Id %x = %d %d\n", pMultiRecFRUInfo->DeviceID, pMultiRecFRUInfo->Offset, pMultiRecFRUInfo->Size);

        /* Set the offset to next multi record */
        MultiRecOffset += MultiRecordHdr.RecordLength + (sizeof (MultiRecordHeader_T) - sizeof(INT8U*));

        NumMultiRec--;
        MultiRecId = va_arg (pArgs, int);

        /* continue till we reach end of multi record */
    }while ((!((MultiRecordHdr.RecordFormatVersionEOL) & 0x80)) && (NumMultiRec));

    va_end (pArgs);

    return 0;
}

/*-----------------------------------------------------
 * InitFRU
 *-----------------------------------------------------*/
int
PDK_InitFRU (int BMCInst)
{

    /* Initilize the Multi Record */

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}


#define OEM_DCMI_FRU_ID 0

/*---------------------------------------------------------------------------
 *@fn 		PDK_DCMIGetFRUDeviceID
 *@brief	Read the FRU device ID which contains product info area
 *			as per FRU 1.1 specification
 *@params	Pointer to FRU Device ID
 *@return 	0 	Sucess
		-1	Failed
 *--------------------------------------------------------------------------*/
int
PDK_DCMIGetFRUDeviceID (INT8U* pDCMIFRUId,int BMCInst)
{
    *pDCMIFRUId = OEM_DCMI_FRU_ID;

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

static INT8U	FruSimBuf [128] = { 0x01, 0x0a, 0x01, 0x83, 0x21, 0x2D, 0x29,0x83, 0x22, 0x2D, 0x23, 0x84, 0x11, 0x12,0x13, 0x14, 0x82, 0x11, 0x12,
								    0x85, 0x11, 0x12, 0x13, 0x14, 0x15,0x84, 'A', 'B', 'C', 'D'};

/*---------------------------------------------------------------------------
 *@fn 		PDK_DCMIReadFRU
 *@brief	Read the FRU information
 *@params	DCMIFRUId - FRU ID
 *		Offset    - Offset into FRU
 *		Len       - Size of FRU data in bytes
 *		*pReadBuf - Pointer to FRU read buffer
 *@return 	0 	Sucess
		-1	Failed
 *--------------------------------------------------------------------------*/
int PDK_DCMIReadFRU (INT8U DCMIFRUId, INT8U Offset, INT8U Len, INT8U* pReadBuf, int BMCInst)
{
    if(g_corefeatures.dcmi_sync_assettag==ENABLED)
    {    
        FRUInfo_T*      pFRUInfo;
 
        /* Get the FRU information for the requested device ID */
        pFRUInfo = GetFRUInfo (DCMIFRUId,BMCInst);
        /* If requested device Id not found */
        if (NULL == pFRUInfo)
        {
            return -1;
        }

        /* Read the FRU data from the FRU device */
        if (-1 == ReadFRUDevice (pFRUInfo, Offset, Len, pReadBuf,BMCInst))
            return -1;
    }
    else
    {
        if (Offset == 4)
        {
            *pReadBuf = 10;
            return 0;
        }
        if (Offset == 80)
        {
            memcpy (pReadBuf, FruSimBuf, Len);
        }
        else
        {
            printf("***************************Unsupported offset*************************\n");
            return -1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------
 *@fn         PDK_DCMIWriteFRU
 *@brief    Write the FRU information
 *@params    DCMIFRUId - FRU ID
 *        Offset    - Offset into FRU
 *        Len       - Size of FRU data in bytes
 *        *pReadBuf - Pointer to FRU read buffer
 *@return   0     Sucess
            -1    Failed
 *--------------------------------------------------------------------------*/
int PDK_DCMIWriteFRU (INT8U DCMIFRUId, INT8U Offset, INT8U Len, INT8U* pReadBuf, int BMCInst)
{
    if(g_corefeatures.dcmi_sync_assettag==ENABLED)
    {
        FRUInfo_T*      pFRUInfo;

        /* Get the FRU information for the requested device ID */
        pFRUInfo = GetFRUInfo (DCMIFRUId,BMCInst);

        /* If requested device Id not found */
        if (NULL == pFRUInfo)
        {
            return -1;
        }

        /* Write the FRU date into the FRU device */
        if(-1== WriteFRUDevice (pFRUInfo, Offset, Len, pReadBuf,BMCInst))
            return -1;
    }
    else
    {
        memcpy (FruSimBuf, pReadBuf, Len);
    }
    return 0;
}

/*
*@fn PDK_FRUGetDeviceAddress
*@brief This function retrieves the FRU Device Address
*@param FruID - FRU Device ID
*@param SlaveAddr - FRU Device Address
*@param BMCInst - BMC Instance
*@return Returns 0 in success
*            Returns -1 on failure
*/
int PDK_FRUGetDeviceAddress(INT8U FruID,INT8U *SlaveAddr,int BMCInst)
{
    unsigned int i=0;

    /* The g_FruTbl has to be filled by OEM as shown below
    FRUAccess_T g_FruTbl [] = 
    {    FRUId,   Device Address    BMCInst
         {0,          0xA0                 ,1 }
    };
    */

    for(i=0;i<sizeof(g_FruTbl)/sizeof(FRUAccess_T);i++)
    {
        if((FruID == g_FruTbl[i].FRUID) && (BMCInst == g_FruTbl[i].BMCInst))
        {
            *SlaveAddr = g_FruTbl[i].SlaveAddr;
            break;
        }
    }

    if(i == sizeof(g_FruTbl)/sizeof(FRUAccess_T))
    {
        return -1;
    }

    return 0;
}


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
 * PDKEEPROM.c
 * EEPROM Access Functions.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 *			Basavaraj Astekar	<basavaraja@ami.com>
 *			Ravinder Reddy		<bakkar@ami.com>
 *******************************************************************/
#define ENABLE_DEBUG_MACROS	0
#include "Types.h"
#include "IPMI_Main.h"
#include "Message.h"
#include "NVRAccess.h"
#include "Debug.h"
#include <fcntl.h>
#include <unix.h>
//#include <flashlib.h>
#include "PDKEEPROM.h"
#include "hal_hw.h"
#include "libtlv.h"
#include "OEMFRU.h"
#include "OEMPSU.h"

#define EEPROM_MAX_PAGE_SIZE		0x30

#define EEPROM_24C64_PAGE_SIZE		0x20
#define EEPROM_24C32_PAGE_SIZE		0x20
#define EEPROM_24C2_PAGE_SIZE		0x08

/* EEPROM Page Size information */
typedef struct {

	INT8U	DeviceType;		/* Device Type as per IPMI specification */
	INT8U	AccessType;		/* Device Access Type byte/word */
	INT8U	PageSize;		/* Page Size for this EEPROM */
	
} EEPROM_Dev_Info_T;

/* EEPROM Information */
typedef struct {

	INT8U	PageSize;
	INT8U	BusNo;
	INT8U	SlaveAddr;
	INT8U	*pData;
	INT16U	Offset;
	INT16U	Len;	
	
} EEPROM_Info_T;

/* Table has information on Supported EEPROM */
EEPROM_Dev_Info_T m_SupEEPROM [] = {
			
	{ 0xF,	 0, 	EEPROM_24C64_PAGE_SIZE },	//24C32 with Page Size of 0x20
	{ 0xE,	 0, 	EEPROM_24C32_PAGE_SIZE },	//24C32 with Page Size of 0x20
	{ 0x9,	 0, 	EEPROM_24C2_PAGE_SIZE }		//24C2  with Page Size of 0x08

	
};

int RWEEPROM (EEPROM_Info_T *pInfo, INT8U RWFlag);

/**
 * GetPageSize
 *  - Gets Page Size for any supported Device Type
 * 
**/
INT8U	
GetPageSize (INT8U	DevType)
{
	unsigned int i = 0;
	
	for (i = 0; i < (sizeof (m_SupEEPROM)/ sizeof (m_SupEEPROM[0])); i++)
	{
		if (m_SupEEPROM[i].DeviceType == DevType) 
		{ 
			return m_SupEEPROM[i].PageSize; 
		}	
	}
	
	return 0;
}

/**
 * ReadWriteEEPROM 
**/
int
ReadWriteEEPROM (INT8U DevType, INT8U BusNumber, INT8U SlaveAddr, INT8U* pData, INT16U EEPROMAddr, INT16U Size, INT8U RWFlag)
{
	EEPROM_Info_T Info;
	
	/* Get Page Size Information */
	Info.PageSize 	= GetPageSize (DevType);
	
	if (Info.PageSize == 0)
	{
		printf ("Unknown EEPROM Type \n");
		return -1;	
	}
	
	/* Fill EEPROM Information */	
	Info.BusNo 		= BusNumber;
	Info.SlaveAddr 	= SlaveAddr;
	Info.pData	 	= pData;
	Info.Offset	 	= EEPROMAddr;		
	Info.Len	 	= Size;
	
	/* Perform R/W operation */	
	return RWEEPROM (&Info, RWFlag);
			
}


/**
 * Generic Read/Write from/to EEPROM 
**/
int
RWEEPROM (EEPROM_Info_T *pInfo, INT8U RWFlag)
{
    INT8U   NumWrite, NumRead, NumByte, Total;
    INT8U   Row;
    INT8U   WriteBuf [EEPROM_MAX_PAGE_SIZE + 2];
    INT8U   ReadBuf  [EEPROM_MAX_PAGE_SIZE];
    INT8	BusName [64];
    int		writeoffset = 0;

    snprintf (BusName,sizeof(BusName), "/dev/i2c-%d", pInfo->BusNo);

    Total = 0;

    while (pInfo->Len > 0)
    {
        // reinitializing the offset for the next write.
        writeoffset = 0;

        if (pInfo->PageSize != EEPROM_24C2_PAGE_SIZE)
        {
            //		printf ("PAGE - 32\n");
            WriteBuf [writeoffset++] = ((pInfo->Offset & 0xFF00 ) >> 8);	//MSB
            WriteBuf [writeoffset++] = ( pInfo->Offset & 0x00FF );			//LSB
        }
        else
        {
            //		printf ("PAGE - 2\n");    		
            WriteBuf [writeoffset++] = pInfo->Offset & 0x00FF;	//byte  		
        }

        Row          =   pInfo->Offset & (pInfo->PageSize - 1);

        if ( (pInfo->Len + Row) > pInfo->PageSize )
        {
            NumByte = ( pInfo->PageSize - Row );
        }
        else
        {
            NumByte = pInfo->Len;
        }

        pInfo->Len -= NumByte;

        if( RWFlag == WRITE_NVR )
        {
            NumWrite = NumByte;
            NumRead  = 0;
            _fmemcpy (&WriteBuf [writeoffset], pInfo->pData, NumWrite);

            /* Write into EEPROM */
            if(g_HALI2CHandle[HAL_I2C_MW] != NULL)
            {
                 if(((ssize_t(*)(char *,u8,u8 *,size_t))g_HALI2CHandle[HAL_I2C_MW]) (BusName,((u8)pInfo->SlaveAddr >> 1), WriteBuf, NumWrite + writeoffset) == -1)
                {
                     IPMI_WARNING ("Error accessing EEPROM\n");
                     return -1;
                }
            }
            else
            {
                IPMI_WARNING ("Error accessing EEPROM\n");
                return -1;
            }

            pInfo->Offset   += NumWrite;
            pInfo->pData    += NumWrite;
            Total        += NumWrite;

            select_sleep(0,10 * 1000);
        }
        else
        {
            NumWrite = 0;
            NumRead  = NumByte;
            int ret = 0;			
            u16 eeprom_size = 0;
            UINT8 buffer[MAX_VPD_SIZE_DELL + 1];
            memset(buffer,0,MAX_VPD_SIZE_DELL + 1);	
            UINT8* pDestBuf = buffer;

            switch(pInfo->BusNo)
            {
                /* Get tlv or standard fru VPD */
                case FRU_EEPROM_BUS_COME:
                case FRU_EEPROM_BUS_BMC:
                case FRU_EEPROM_BUS_PSUB:
                case FRU_EEPROM_BUS_SYS:
                case FRU_EEPROM_BUS_FIOB:
                case FRU_EEPROM_BUS_FAN1:
                case FRU_EEPROM_BUS_FAN2:
                case FRU_EEPROM_BUS_FAN3:
                case FRU_EEPROM_BUS_FAN4:
                case FRU_EEPROM_BUS_FAN5:
                case FRU_EEPROM_BUS_FCB:
                    //TLV EEPROM Format
                    eeprom_size = get_eeprom_size(pInfo->BusNo);
                    if(eeprom_size <= 0) {
                        IPMI_WARNING("Invalid EEPROM size for TLV on bus %d\n", pInfo->BusNo);
                        return -1;
                    }

                    ret = tlv_eeprom_is_valid(pInfo->BusNo, (pInfo->SlaveAddr >> 1), eeprom_size);
                    if (ret == TLV_SUCCESS)
                    {
                        TransferDellVpdInfoToStandardFormat(pInfo->BusNo, (pInfo->SlaveAddr >> 1), eeprom_size, pDestBuf);
                    }
                    else
                    {
                        IPMI_WARNING ("TLV eeprom is not valid\n");
                        return -1;
                    }
                    _fmemcpy ( pInfo->pData, &pDestBuf[pInfo->Offset], NumRead);
                    break;

                default:
                    //Standard IPMI FRU Format
                    /* Read from EEPROM */
                    if(g_HALI2CHandle[HAL_I2C_RW] != NULL)
                    {
                        if(((int(*)(char *,u8,u8 *,u8 *,size_t,size_t))g_HALI2CHandle[HAL_I2C_RW]) (BusName,((u8)pInfo->SlaveAddr >> 1), WriteBuf, ReadBuf, NumWrite + writeoffset, NumRead) < 0)
                        {
                            IPMI_WARNING ("Error accessing EEPROM\n");
                            return -1;
                        }
                    }
                    else
                    {
                        IPMI_WARNING ("Error accessing EEPROM\n");
                        return -1;
                    }
                _fmemcpy ( pInfo->pData, &ReadBuf [0], NumRead);
                break;
            }
            
#if 0
    printf("BusName = %s\n",BusName);
    printf("SlaveAddr = %d\n",pInfo->SlaveAddr);
    printf("NumRead = %d\n",NumRead);
#endif

            pInfo->Offset   += NumRead;
            pInfo->pData    += NumRead;
            Total           += NumRead;
        }   
    }
    return Total;
}

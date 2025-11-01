/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc.
*** **
*** All Rights Reserved.
*** **
*** **
*** File Name: OEMFRU.c
*** Description: Implements helper functions for OEM FRU handling,
*** such as converting Dell-specific VPD format to standard IPMI format.
*** **
***************************************************************************
***************************************************************************
**************************************************************************/
#include "Types.h"
#include "OSPort.h"
#include "hal_hw.h"
#include "OemDefs.h"
#include "OEMFRU.h"
#include "OEMDBG.h"
#include <time.h>
#include "libtlv.h"


/**
 * @brief Gets the EEPROM size for a given I2C bus number.
 *
 * This function returns the pre-defined size of an EEPROM based on the
 * I2C bus it resides on. This is used to determine how much data to
 * read for TLV (Tag-Length-Value) parsing.
 *
 * @param BusNo The I2C bus number of the FRU EEPROM.
 * @return The size of the EEPROM in bytes, or -1 if the bus number is not recognized.
 */
u16 get_eeprom_size(INT8U BusNo)
{
    u16 eeprom_size = 0;
    switch(BusNo)
    {
        case FRU_EEPROM_BUS_COME:
        case FRU_EEPROM_BUS_BMC:
        case FRU_EEPROM_BUS_PSUB:
        case FRU_EEPROM_BUS_FAN1:
        case FRU_EEPROM_BUS_FAN2:
        case FRU_EEPROM_BUS_FAN3:
        case FRU_EEPROM_BUS_FAN4:
        case FRU_EEPROM_BUS_FAN5:
        case FRU_EEPROM_BUS_FCB:
            eeprom_size = EEPROM_24C64_SIZE;
            break;
        case FRU_EEPROM_BUS_SYS:
        case FRU_EEPROM_BUS_FIOB:
            eeprom_size = EEPROM_24C02_SIZE;
            break;
        default:
            return -1;
    }
    return eeprom_size;
}

/**
 * @brief Calculates the 8-bit checksum for a block of memory.
 *
 * This function computes a checksum for a given memory range. The checksum is
 * calculated such that the sum of all bytes in the range plus the checksum
 * byte itself equals 0 (modulo 256). This is a common method for ensuring
 * data integrity in FRU data areas.
 *
 * @param pStart Pointer to the start of the data block.
 * @param pEnd Pointer to the end of the data block (inclusive).
 * @return The 8-bit checksum value.
 */
static UINT8 OEM_zerosum(UINT8* pStart, UINT8* pEnd){
    UINT8 sum=0x0;
    while(pStart<=pEnd){
        sum+=*pStart++;
    }
    return (UINT8)(0xFF-sum+1);
}

/**
 * @brief Converts a date string to an IPMI FRU timestamp.
 *
 * This function parses a date string in the format "MM/DD/YYYY hh:mm:ss" and
 * converts it into the IPMI FRU timestamp format, which is the number of
 * minutes elapsed since 00:00:00, January 1, 1996.
 *
 * @param date_str The input date string.
 * @return The number of minutes since the IPMI epoch, or 0 on failure.
 */
static unsigned long date_str_to_ipmi_timestamp(const char* date_str)
{
    struct tm time_info = {0};
    time_t raw_time;
    unsigned long minutes;

    // Parse the string "MM/DD/YYYY hh:mm:ss"
    if (sscanf(date_str, "%d/%d/%d %d:%d:%d",
               &time_info.tm_mon, &time_info.tm_mday, &time_info.tm_year,
               &time_info.tm_hour, &time_info.tm_min, &time_info.tm_sec) != 6) {
        printf("ERROR: Failed to parse date string: %s\n", date_str);
        return 0; // Parsing failed
    }

    // struct tm adjustments
    time_info.tm_year -= 1900; // Years since 1900
    time_info.tm_mon -= 1;     // Months since January (0-11)

    // Convert to time_t (seconds since epoch 1970/01/01)
    raw_time = timegm(&time_info);
    if (raw_time == -1) {
        return 0;
    }

    // IPMI FRU timestamp is minutes from 00:00:00 1/1/1996.
    // Seconds from 1/1/1970 to 1/1/1996 is 820454400.
    minutes = (raw_time < SECONDS_FROM_EPOCH) ? 0 : ((raw_time - SECONDS_FROM_EPOCH) / 60);
    return minutes;
}

/**
 * @brief Converts Dell-specific TLV VPD format to standard IPMI FRU format.
 *
 * This function reads Vital Product Data (VPD) from an EEPROM in Dell's
 * proprietary TLV (Tag-Length-Value) format, extracts key fields like
 * serial number, part number, and manufacturing date, and then repacks
 * them into the standard IPMI FRU information format. The resulting
 * standard-compliant data is written to the destination buffer.
 *
 * @param BusNo The I2C bus number of the EEPROM.
 * @param SlaveAddr The I2C slave address of the EEPROM.
 * @param eeprom_size The total size of the EEPROM.
 * @param pDestBuf Pointer to the destination buffer to store the converted standard IPMI FRU data.
 * @return The total size of the generated IPMI FRU data in bytes, or a negative value on error.
 */
int TransferDellVpdInfoToStandardFormat(INT8U BusNo, INT8U SlaveAddr, u16 eeprom_size, UINT8* pDestBuf)
{   
    VPD_STANDARD_FORMAT_T VpdStandard;
    TLV_Dell_VPD_T  TlvVpd;
    VPDMSect_T *pMSect=NULL;
    UINT8* pWalker = pDestBuf;
    int i=0, FieldLength=0;
    int ret = 0;
    u8 actual_len = 0;
    memset((void*)&VpdStandard,0,sizeof(VPD_STANDARD_FORMAT_T));
    memset((void*)&TlvVpd,0,sizeof(TLV_Dell_VPD_T));  
    /*Specify CommonHeader after all sector is filled,set pWalker to SIZE_VPD_COMMON_HEADER*/ 
    pWalker += SIZE_VPD_COMMON_HEADER;    
       
    /*Specify BoardInfo */          
    VpdStandard.BoardInfo.FormatVer = VPD_FROMAT_VER(0x1);      
    FieldLength = VPD_BOARD_INFO_FIXED_SIZE;//Board field Default
    VpdStandard.BoardInfo.LanguageCode = LANGUAGE_CODE;//force set to this language code so ipmitool can parse it.

    //Get TLV_CODE_MANUF_DATE
    ret = tlv_eeprom_read(BusNo, SlaveAddr, eeprom_size, TLV_CODE_MANUF_DATE, TlvVpd.MFG, sizeof(TlvVpd.MFG), &actual_len);
    if (ret == TLV_SUCCESS)
    {       
        // Ensure the string is null-terminated before passing to sscanf to prevent a buffer overflow.
        char date_buf[DELL_VPD_MFG_DATE_BUF_SIZE];
        memcpy(date_buf, TlvVpd.MFG, DELL_VPD_MFG_DATE_LEN);
        date_buf[DELL_VPD_MFG_DATE_LEN] = '\0';
        // representing minutes since 1/1/1996.
        unsigned long mfg_minutes = date_str_to_ipmi_timestamp(date_buf);
        VpdStandard.BoardInfo.MfgDateTime[0] = (UINT8)(mfg_minutes & 0xFF);
        VpdStandard.BoardInfo.MfgDateTime[1] = (UINT8)((mfg_minutes >> 8) & 0xFF);
        VpdStandard.BoardInfo.MfgDateTime[2] = (UINT8)((mfg_minutes >> 16) & 0xFF);
    }
    else
    {
        printf("ERROR: get TLV_CODE_MANUF_DATE fail\n");
        return -1;
    }
    
    VpdStandard.BoardInfo.ManufactureName.SectTypeLen = 0;
    VpdStandard.BoardInfo.ManufactureName.pSectData=NULL;        
    //for ManufactureName section, SectTypeLen =0 will not fill in with no SectData followed
    FieldLength += 1;

    VpdStandard.BoardInfo.ProductName.SectTypeLen = 0;
    VpdStandard.BoardInfo.ProductName.pSectData=NULL;        
    //for ProductName section, SectTypeLen =0 will not fill in with no SectData followed
    FieldLength += 1;

    //Get TLV_CODE_SERIAL_NUMBER
    ret = tlv_eeprom_read(BusNo, SlaveAddr, eeprom_size, TLV_CODE_SERIAL_NUMBER, TlvVpd.SN, sizeof(TlvVpd.SN), &actual_len);
    if (ret == TLV_SUCCESS)
    {
        VpdStandard.BoardInfo.SerialNum.SectTypeLen = SET_VPD_DATA_TYPE_LCODE(sizeof(TlvVpd.SN));    
        VpdStandard.BoardInfo.SerialNum.pSectData = TlvVpd.SN;
        FieldLength += (1 + sizeof(TlvVpd.SN));              
    }
    else
    {
        printf("ERROR: get TLV_CODE_SERIAL_NUMBER fail\n");
        return -1;
    }
    if(g_OEMDebugArray[OEM_DEBUG_Item_FRU] > 1){
        printf("SN :: ");
        for(i = 0; i < (int)(sizeof(TlvVpd.SN)); i++){
            printf("%c",TlvVpd.SN[i]);          
        }
        printf("\n");
    }

    //Get TLV_CODE_PART_NUMBER
    ret = tlv_eeprom_read(BusNo, SlaveAddr, eeprom_size, TLV_CODE_PART_NUMBER, TlvVpd.PartNum, sizeof(TlvVpd.PartNum), &actual_len);
    if (ret == TLV_SUCCESS)
    {
        VpdStandard.BoardInfo.PartNum.SectTypeLen = SET_VPD_DATA_TYPE_LCODE(sizeof(TlvVpd.PartNum));    
        VpdStandard.BoardInfo.PartNum.pSectData = TlvVpd.PartNum;
        FieldLength += (1 + sizeof(TlvVpd.PartNum));                        
    }
    else
    {
        printf("ERROR: get TLV_CODE_PART_NUMBER fail\n");
        return -1;
    }
    if(g_OEMDebugArray[OEM_DEBUG_Item_FRU] > 1)
    {
        printf("PartNum :: ");  
        for(i = 0; i < (int)(sizeof(TlvVpd.PartNum)); i++){
            printf("%c",TlvVpd.PartNum[i]);         
        }
        printf("\n");   
    }

    VpdStandard.BoardInfo.FRUfileID.SectTypeLen = 0;        
    VpdStandard.BoardInfo.FRUfileID.pSectData  = NULL;      
    //for mandatory section, SectTypeLen =0 will fill in with no SectData followed      
    FieldLength += 1; 

    VpdStandard.BoardInfo.CustomizedInfo.SectTypeLen = 0;       
    VpdStandard.BoardInfo.CustomizedInfo.pSectData =NULL;       
    //for customizable section, SectTypeLen =0 will not fill in with no SectData followed
    FieldLength += ((VpdStandard.BoardInfo.CustomizedInfo.SectTypeLen==0)?0:(1 + VpdStandard.BoardInfo.CustomizedInfo.SectTypeLen));
    
    VpdStandard.BoardInfo.IsFieldInfoEnd = IPMI_FRU_FIELD_END;
    VpdStandard.BoardInfo.PaddingBytes = (UINT8)(IPMI_FRU_AREA_ALIGNMENT - (FieldLength + IPMI_FRU_END_AND_CHECKSUM_SIZE) % IPMI_FRU_AREA_ALIGNMENT);// include 2byte for IsFieldInfoEnd ZeroSum
    VpdStandard.BoardInfo.ZeroSum = 0x0; //init, will be calculated 

    //calculate the final length
    FieldLength += (1+VpdStandard.BoardInfo.PaddingBytes+1);
    VpdStandard.BoardInfo.Length = (UINT8)(FieldLength>>3);

    /*Fill BoardInfo into pDestBuf to create standard format VPD, including mandatory sect and custom sector*/      
    memcpy(pWalker,(void*)&VpdStandard.BoardInfo,6);            
    pWalker += VPD_BOARD_INFO_FIXED_SIZE;            
    pMSect = &VpdStandard.BoardInfo.ManufactureName;
    for(i = 0; i < IPMI_FRU_BOARD_MANDATORY_FIELD_COUNT; i++){
        //Fill 5 mandatory section including manufacture, product name, SN, part number, fru file id.        
        //For mandatory section, Byte "SectTypeLen =0" will be filled into VPD with no SectData followed        
        //If not, fru print will display "BoardExtra 0000000000...0000"     
        *pWalker++ = pMSect->SectTypeLen;           
        if((pMSect->SectTypeLen & SECT_TYPE_LEN_MASK) != 0)
            memcpy(pWalker,pMSect->pSectData,(pMSect->SectTypeLen & SECT_TYPE_LEN_MASK));
        pWalker += (pMSect->SectTypeLen & SECT_TYPE_LEN_MASK);
        pMSect ++;      
    }           

    //Additional custom Mfg.info
    while((pMSect->pSectData!=NULL)&&(0!=(pMSect->SectTypeLen & SECT_TYPE_LEN_MASK))&&(pMSect->SectTypeLen != VpdStandard.BoardInfo.IsFieldInfoEnd)){
        *pWalker++ = pMSect->SectTypeLen;       
        memcpy(pWalker,pMSect->pSectData,(pMSect->SectTypeLen & SECT_TYPE_LEN_MASK));
        pWalker += (pMSect->SectTypeLen & SECT_TYPE_LEN_MASK);
        pMSect ++;      
    } 
         
    *pWalker++ = VpdStandard.BoardInfo.IsFieldInfoEnd;          
    while(VpdStandard.BoardInfo.PaddingBytes--) {       
        *pWalker++ =0x00;//fill unused bytes as 0   
        if((pWalker - pDestBuf)> MAX_VPD_SIZE_DELL){        
            printf("ERROR: VPD data truncated for too long\n");     
            return MAX_VPD_SIZE_DELL;               
        }           
    }   

    *pWalker = OEM_zerosum(pWalker-(VpdStandard.BoardInfo.Length<<3)+1,pWalker-1);  
    if(g_OEMDebugArray[OEM_DEBUG_Item_FRU] > 0){
        printf("%x",*pWalker);
    }

    /*Specify CommonHeader*/
    VpdStandard.CommonHeader.FormatVer = VPD_FROMAT_VER(0x1);       
    VpdStandard.CommonHeader.OffsetInternalUseArea = 0;       
    VpdStandard.CommonHeader.OffsetChassisInfoArea = 0;         
    VpdStandard.CommonHeader.OffsetBoardInfoArea = (UINT8)(SIZE_VPD_COMMON_HEADER>>3);      
    VpdStandard.CommonHeader.OffsetProductInfoArea = 0;     
    VpdStandard.CommonHeader.OffsetMultiRecordArea = 0;     
    VpdStandard.CommonHeader.Pad = 0;       
    VpdStandard.CommonHeader.HeaderZeroSum = OEM_zerosum(&VpdStandard.CommonHeader.FormatVer,&VpdStandard.CommonHeader.Pad);
    memcpy(pDestBuf,(void*)&VpdStandard.CommonHeader,SIZE_VPD_COMMON_HEADER);
    
    /* Fill the rest of the buffer with 0xFF */
    if ((MAX_VPD_SIZE_DELL - (pWalker - pDestBuf)) > 0) {
        memset(pWalker + 1, 0xff, MAX_VPD_SIZE_DELL - (pWalker - pDestBuf));
    }

    return (pWalker-pDestBuf+1);
}

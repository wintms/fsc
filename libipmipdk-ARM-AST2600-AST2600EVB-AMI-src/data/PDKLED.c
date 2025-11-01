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
 * PDKLED.c
 * LED Access Functions.
 *
 *  Author: Gowtham Shanmukham <gowthams@ami.com>
 ******************************************************************/
#define ENABLE_DEBUG_MACROS     0

#include "Types.h"
#include "OSPort.h"
#include "Debug.h"
#include "PDKHooks.h"
#include "API.h"
#include "gpioifc.h"
#include "SharedMem.h"
#include "Indicators.h"
#include "LEDMap.h"
#include "NVRAccess.h"
#include "Platform.h"
#include "IPMIConf.h"
#include "hal_hw.h"

/*--------------------------------------------------------------------------
 * @fn  PDK_SetupLEDMap
 * @brief Setup pointers to correct LED Map for given chassis/board
 * @return 0 if success
 *         -1 if failure
 *--------------------------------------------------------------------------*/
int
PDK_PlatformSetupLED (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }    
    return 0;
}


/*--------------------------------------------------------------------------
 * @fn  PDK_GlowLED
 * @brief Glow or turn of an LED.
 * @param LEDNum        LED number to turn on or turn off.
 * @param IsOn          TRUE to turn ON. FALSE to turn off.
 *--------------------------------------------------------------------------*/
void
PDK_GlowLED (INT8U LEDNum, INT8U n, int BMCInst)
{
	  INT8U SlaveAddr,BusNo;
	  ssize_t read_count,write_count;
	  char Read_Buf[1024],Write_Buf[1024];
	  char BusName[64];
	  BMCSharedMem_T*  hBMCSharedMem;
	  IndicatorInfo_T  *LEDInfo;
	  BMCInfo_t *pBMCInfo = (BMCInfo_t *)&g_BMCInfo[BMCInst];

	  memset(BusName,0,sizeof(BusName));
	  memset(Read_Buf,0,1024);
	  memset(Write_Buf,0,1024);


	  if (LEDNum >= MAX_LED_NUM)
	    return;

	  hBMCSharedMem = BMC_GET_SHARED_MEM(BMCInst);

	  LOCK_BMC_SHARED_MEM(BMCInst);

	  LEDInfo = (IndicatorInfo_T*)&((BMCSharedMem_T*)hBMCSharedMem)->LEDInfo[LEDNum];


	  if(n != 0)
	  {
	    if(LEDInfo->NumSec != 0)
	      Write_Buf[0] = 0xFE;
	    else
	      Write_Buf[0] = 0xFF;
	  }
	  else
	    Write_Buf[0] = 0xFF;

	  if(pBMCInfo->Msghndlr.ChassisIdentifyForce == TRUE)
	  {
	    LEDInfo->NumSec = 0xFF;
	    LEDInfo->Enable = 1;
	    LEDInfo->Count = 0;
	  }

	  UNLOCK_BMC_SHARED_MEM(BMCInst);

	  SlaveAddr = 0x70;
	  BusNo = 0x01;
	  read_count = write_count = 1;
	  snprintf(BusName,sizeof(BusName),"/dev/i2c%d",BusNo);



	  if(g_HALI2CHandle[HAL_I2C_RW] != NULL)
	  {
	    if(0 > ((int(*)(char *,u8,u8 *,u8 *,size_t,size_t))g_HALI2CHandle[HAL_I2C_RW]) (BusName,(SlaveAddr >> 1),(u8 *)&Write_Buf[0],
	                              (u8 *)&Read_Buf[0],write_count,read_count))
	    {
	      /* Erron in reading */
	      return;
	    }
	  }
}

/*--------------------------------------------------------------------
 * @fn PDK_GlowFaultLED
 * @brief This function is invoked to manage the front fan failure led
 * @param Fault - Indicates desired fault type to take action on:
 *                LED_FAULT_TEMP   LED_FAULT_FAN     LED_FAULT_PS
 *                LED_FAULT_CPU    LED_FAULT_HDD
 *                LED_FAULT_ECC_CE LED_FAULT_ECC_UE
 * @param State - Caller indicates desired state
 *                LED_STATE_RESET - all off and reset
 *                LED_STATE_OFF   - off from one source
 *                LED_STATE_ON    - on from one source
 *--------------------------------------------------------------------*/
void
PDK_GlowFaultLED (int Fault, int State, INT8U Data, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Fault=Fault;
	State=State;
	Data=Data;
    }
}

/*-----------------------------------------------------------------
 * @fn PDK_UpdatePowerLEDStatus
 * @brief This is provided to implement the platform specific
 *        update Power LED status procedure according to Operational
 *        State Machine.
 *
 * @param pFruObj  - Pointer of Operational State FRU object. 
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_UpdatePowerLEDStatus (OpStateFruObj_T *pFruObj, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pFruObj=pFruObj;
    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_UpdateFaultLEDStatus
 * @brief This is provided to implement the platform specific
 *        update Fault LED status procedure according to Aggregated
 *        Fault Sensor.
 *
 * @param FaultState - State according to Aggregated Fault Sensor.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_UpdateFaultLEDStatus (INT8U FaultState, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	FaultState=FaultState;
    }
    return;
}


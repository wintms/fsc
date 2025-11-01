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
 * pdkhooks.c
 * Hooks that are invoked at different points of the Firmware
 * execution.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 ******************************************************************/
#define ENABLE_DEBUG_MACROS     0
#include "Types.h"
#include "Debug.h"
#include "PDKHooks.h"
#include "API.h"
#include "IPMIDefs.h"
#include "IPMI_Chassis.h"
#include "IPMI_SEL.h"
#include "IPMI_SDRRecord.h"
#include "IPMI_IPM.h"
#include "SharedMem.h"
#include "LEDMap.h"
#include "Indicators.h"
#include "NVRAccess.h"
#include "Platform.h"
#include "PDKInt.h"
#include "PDKCmds.h"
#include "PDKFRU.h"
#include "PDKSensor.h" 
#include "WDT.h"
#include "SensorAPI.h"
#include "kcsifc.h"
#include "IPMI_KCS.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "IPMIConf.h"
#include "GUID.h"
#include "Pnm.h"
#include "SDRFunc.h"
#include "SEL.h"
#include "hal_hw.h"
#include "featuredef.h"
#include "kcs_ioctl.h"
#include "bt_ioctl.h"

#include "OEMSysInfo.h"

#define GET_POWER_STATUS    1
#define GET_PS_STATUS       2
#define READ_SENSOR         3
#define GET_CPU_STATUS      4
#define GET_FAN_STATUS      5

#define MMC_ADDR 0x40

#define GPIO_SSIF_ALERT_N 174

#define IPMI_EVENT_TYPE_BASE 0x09


#define PSGOOD_WAIT_TIME    3
#define IPV6_DEFAULT_FF      0xff

#define MAX_SNOOP_SIZE 1024
#define SNOOP_DELAY_TIME 60

PDK_ChannelInfo_T g_pdkChannelInfo;
INT8U 			  g_pdkIsPktFromVLAN = FALSE;
static INT8U 	  gPDKLastPowerEvent = PDK_LAST_POWER_DOWN_VIA_AC_FAILURE;

// This is set to TRUE when the Init Agent has just rearmed all sensors.
bool  g_bInitAgentRearmed = FALSE;

int g_pdkSnoopCounter=0;
int g_pdkSnoopFlag=0;


#define USE_PECI_TO_READ_CPU_MEMORY_SENSOR_VALUEx
#ifdef USE_PECI_TO_READ_CPU_MEMORY_SENSOR_VALUE
#define CPU_SEL                                                     0x01
#define CPU_0_1_MEMORY_CHANNEL                                      0x0F
#define CPU_2_3_MEMORY_CHANNEL                                      0x00
#define PNM_PECI_ME_SLAVE_ADDRESS                                   0x88
#define PNM_PECI_PROXY_FNLUN                                        ((INT8U)(0x2E << 2))
#define PNM_DATA_START_OFFSET                                       0x07
#define CMD_PNM_OEM_PECI_PROXY_GET_CPU_AND_MEMORY_TEMPERATURE       0x4B
#define LIMITATION_TO_GET_SENDSOR_VALUE                             0x3
#define CMD_PNM_OEM_PECI_PROXY_GET_CPU_AND_MEMORY_TEMPERATURE_LEN   20

#define PNM_COMPLETE_CODE_LEN                           		    0x01
#define PNM_INTEL_MANUFACTURER_ID_LEN                               0x03

#define CPU_0_SENSORNUM             0x31
#define CPU_1_SENSORNUM             0x32
#define CPU_2_SENSORNUM             0x33
#define CPU_3_SENSORNUM             0x34
#define CPU_0_MEMORY_CH_0_SENSORNUM 0x35
#define CPU_0_MEMORY_CH_1_SENSORNUM 0x36
#define CPU_0_MEMORY_CH_2_SENSORNUM 0x37
#define CPU_0_MEMORY_CH_3_SENSORNUM 0x38
#define CPU_1_MEMORY_CH_0_SENSORNUM 0x39
#define CPU_1_MEMORY_CH_1_SENSORNUM 0x40
#define CPU_1_MEMORY_CH_2_SENSORNUM 0x41
#define CPU_1_MEMORY_CH_3_SENSORNUM 0x42
#define CPU_2_MEMORY_CH_0_SENSORNUM 0x43
#define CPU_2_MEMORY_CH_1_SENSORNUM 0x44
#define CPU_2_MEMORY_CH_2_SENSORNUM 0x45
#define CPU_2_MEMORY_CH_3_SENSORNUM 0x46
#define CPU_3_MEMORY_CH_0_SENSORNUM 0x47
#define CPU_3_MEMORY_CH_1_SENSORNUM 0x48
#define CPU_3_MEMORY_CH_2_SENSORNUM 0x49
#define CPU_3_MEMORY_CH_3_SENSORNUM 0x50

enum
{
    CPU_0_INDEX,
    CPU_1_INDEX,
    CPU_2_INDEX,
    CPU_3_INDEX,
    CPU_0_MEMORY_CH_0_INDEX,
    CPU_0_MEMORY_CH_1_INDEX,
    CPU_0_MEMORY_CH_2_INDEX,
    CPU_0_MEMORY_CH_3_INDEX,
    CPU_1_MEMORY_CH_0_INDEX,
    CPU_1_MEMORY_CH_1_INDEX,
    CPU_1_MEMORY_CH_2_INDEX,
    CPU_1_MEMORY_CH_3_INDEX,
    CPU_2_MEMORY_CH_0_INDEX,
    CPU_2_MEMORY_CH_1_INDEX,
    CPU_2_MEMORY_CH_2_INDEX,
    CPU_2_MEMORY_CH_3_INDEX,
    CPU_3_MEMORY_CH_0_INDEX,
    CPU_3_MEMORY_CH_1_INDEX,
    CPU_3_MEMORY_CH_2_INDEX,
    CPU_3_MEMORY_CH_3_INDEX,
    END_OF_CPU_MEMORY_LIST
};

INT8U blGetMessage = 0;
INT8U GetSendsorValCounter = 0;
INT8U CPU_MEMORY_SENSOR_VAL[END_OF_CPU_MEMORY_LIST];
INT8U CPU_TJMAX[4] = {0,0,0,0};
INT8U blGetMessage_cpu = 0;
INT8U recv_cpu_tjmax = 1;

/**
*@fn CalculateCheckMsgSum
*@brief Calculates the checksum
*@param Pkt Pointer to the data for the checksum to be calculated
*@param Len Size of data for checksum calculation
*@return Returns the checksum value
*/
INT32U
CalculateCheckMsgSum (_FAR_ INT8U* Pkt, INT32U Len)
{
    INT8U	Sum;
    INT32U	i;

    /* Get Checksum 2 */
    Sum = 0;
    for (i = 0; i < Len; i++)
    {
        Sum += Pkt [i];
    }
    return (INT8U)(0xFF & (0x100 - Sum));
}

/*
 * @fn PDK_GetCPUTJMAX
 * @brief This function is used to get CPU TjMAX value
 * @CPUNum - CPU number
 */
void
PDK_GetCPUTJMAX(int BMCInst,int CPUNum)
{

	MsgPkt_T MsgPkt;
	_FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	INT8U ResIndex=0;
	INT8U index=0,chksumlen=0;
	int retry=3;
	
	if (pBMCInfo->IpmiConfig.NodeMgrSupport)
	{
		 if (blGetMessage_cpu == 0)
		{
							
			memset(MsgPkt.Data, 0, MSG_PAYLOAD_SIZE);
			/* Construct the request packet for Get CPU and Memory temperature Information*/
			MsgPkt.NetFnLUN = NETFN_APP << 2;
				MsgPkt.Cmd		= CMD_SEND_MSG;
				MsgPkt.Data[0] = pBMCInfo->SMLINKIPMBCh; /*Channel Number*/
				MsgPkt.Data[1] = PNM_PECI_ME_SLAVE_ADDRESS;
				MsgPkt.Data[2] = NETFN_SENSOR << 2;
				MsgPkt.Data[3] = (INT8U)CalculateCheckMsgSum (&MsgPkt.Data[1], 2);
				MsgPkt.Data[4] = pBMCInfo->IpmiConfig.BMCSlaveAddr;
				MsgPkt.Data[5] = 0x01;
				MsgPkt.Data[6] = 0x2d;
				
				if(CPUNum == 0)
				MsgPkt.Data[7] = 0x30;
				else if(CPUNum == 1)
				MsgPkt.Data[7] = 0x31;
				else if(CPUNum == 2)
				MsgPkt.Data[7] = 0x32;
				else if(CPUNum == 3)
				MsgPkt.Data[7] = 0x33;

				index=8;
				chksumlen=index-4;
				MsgPkt.Data[8]=(INT8U)CalculateCheckMsgSum (&MsgPkt.Data[4], chksumlen);
				MsgPkt.Size = 9;				
				/* Execute the IPMI send message command*/
				API_ExecuteCmd (&MsgPkt, BMCInst);				
				if (CC_SUCCESS == ((SendMsgRes_T*)(MsgPkt.Data))->CompletionCode)
				{
					blGetMessage_cpu = 1;
					
				}
				else
				{
					
					/*IPMI_WARNING ("%s : Failed to sent CPU and Memory temperature\n",__FUNCTION__);*/
				}
			
		}
		else
		{
			while(retry>0)
			{
				MsgPkt.NetFnLUN = (NETFN_APP << 2) | 0x00;
				MsgPkt.Cmd = CMD_GET_MSG;
				MsgPkt.Size = 0; // no request data
							
				API_ExecuteCmd (&MsgPkt, BMCInst);
		
				if (CC_SUCCESS == ((GetMsgRes_T*)(MsgPkt.Data))->CompletionCode)
				{
					CPU_TJMAX[CPUNum] = MsgPkt.Data[ResIndex+PNM_DATA_START_OFFSET+PNM_COMPLETE_CODE_LEN+1];				
					blGetMessage_cpu = 0;
					return;
				}
				else
				{
				
				blGetMessage_cpu = 0;
				retry--;
				sleep(1);
				
				}
			}
			/* faile to recv value*/
			IPMI_WARNING ("%s : Failed to get CPU and Memory temperature\n", __FUNCTION__);
			recv_cpu_tjmax = 0;
		}
	}
	return;
}


#endif /* USE_PECI_TO_READ_CPU_MEMORY_SENSOR_VALUE */

int PDK_GetLANConfigurations(LANIFCConfig_T *LanIfcConfig,int BMCInst)
{
    int NIC_Count = GetMacrodefine_getint("CONFIG_SPX_FEATURE_GLOBAL_NIC_COUNT",0);

    if(0)
    {
	BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    if (NIC_Count == 0x4)
    {
        LANIFCConfig_T m_LANConfigTbl4 [MAX_LAN_CHANNELS] =
        {
            /*Ifcname,         Channel Type              Enable     ,Channel Num          EthIndex          Status  */
            {"eth0",          LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          },
            {"eth1",          LAN_RMCP_CHANNEL2_TYPE,    0,         8,                    1,                0          },
            {"eth2",          LAN_RMCP_CHANNEL3_TYPE,    0,         7,                    2,                0          },
            {"eth3",          LAN_RMCP_CHANNEL4_TYPE,    0,         9,                    3,                0          }
        };
        memcpy(LanIfcConfig,m_LANConfigTbl4,sizeof(m_LANConfigTbl4));
    }
    else if ( NIC_Count == 0x3)
    {
        LANIFCConfig_T m_LANConfigTbl3 [MAX_LAN_CHANNELS] =
        {
            /*Ifcname,         Channel Type              Enable     ,Channel Num          EthIndex          Status  */
            {"eth0",          LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          },
            {"eth1",          LAN_RMCP_CHANNEL2_TYPE,    0,         8,                    1,                0          },
            {"eth2",          LAN_RMCP_CHANNEL3_TYPE,    0,         7,                    2,                0          },
            {"bond0",         LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          }
        };
        memcpy(LanIfcConfig,m_LANConfigTbl3,sizeof(m_LANConfigTbl3));
    }
    else if (NIC_Count == 0x2)
    {
        LANIFCConfig_T m_LANConfigTbl2 [MAX_LAN_CHANNELS] =
        {
            /*Ifcname,         Channel Type              Enable     ,Channel Num          EthIndex          Status  */
            {"eth0",          LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          },
            {"eth1",          LAN_RMCP_CHANNEL2_TYPE,    0,         8,                    1,                0          },
            {"bond0",         LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          }
        };
        memcpy(LanIfcConfig,m_LANConfigTbl2,sizeof(m_LANConfigTbl2));
    }
    else
    {
        LANIFCConfig_T m_LANConfigTbl1 [MAX_LAN_CHANNELS] =
        {
            /*Ifcname,         Channel Type              Enable     ,Channel Num          EthIndex          Status  */
            {"eth0",          LAN_RMCP_CHANNEL1_TYPE,    0,         1,                    0,                0          },
        };
        memcpy(LanIfcConfig,m_LANConfigTbl1,sizeof(m_LANConfigTbl1));
    }

    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_BeforeCreatingTasks
 *
 * @brief This function is called is before the tasks are created.
 * You can perform any initialization required for the proper
 * functioning of any of the standard or OEM tasks. You can even
 * create an OEM task here.
 *
 * @return  0
 *-----------------------------------------------------------------*/
int
PDK_BeforeCreatingTasks (int BMCInst)
{
    /* Porting tasks can be created here */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_AfterCreateTask
 * @brief This function is called just after all the tasks are created.
 * @return 	0 	if Success
 *			-1	if Error
 * CHECK_POINT:	03
 *-----------------------------------------------------------------*/
int
PDK_AfterCreatingTasks (int BMCInst)
{
    /* Porting tasks can be created here */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_OnTaskStartup
 *
 * @brief This function is called on the startup of a particular task.
 * Only core tasks will invoke this hook. The id passed as input to
 * this function is the ID of the task that is starting up. This hook
 * is not invoked for any of the OEM tasks. For OEM tasks you need to
 * perform the startup functions in the task itself.
 *
 * @param	Id - Task ID
 *				SMBIFC_TASK_PRIO
 *				LAN_TASK_PRIO
 *				DHCP_TASK_PRIO
 *				SHMEM_MUTEX_PRIO
 *				USB_TASK_PRIO
 *				RVDP_TASK_PRIO
 *				UKVM_TASK_PRIO
 *				SM_TASK_PRIO
 *				SERIAL_R_TASK_PRIO
 *				SOL_TASK_PRIO
 *				MSG_HNDLR_PRIO
 *				TIMER_TASK_PRIO
 *				CHASSIS_TASK_PRIO
 *				IPMB_TASK_PRIO
 *				SMBUS_TASK_PRIO
 *				BT_TASK_PRIO
 *				KCS1_TASK_PRIO
 *				SERIAL_TASK_PRIO
 *				ICMB_TASK_PRIO
 *				NVR_TASK_PRIO
 *				PEF_TASK_PRIO
 *				SENSOR_MONITOR_PRIO
 *				MDL_TASK_PRIO
 *-----------------------------------------------------------------*/
void
PDK_OnTaskStartup (INT8U Id,int BMCInst)
{

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Id=Id;
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_LPCReset
 * @brief This function is called by the core during LPC Reset
 *------------------------------------------------------------------------*/
void
PDK_LPCReset (int BMCInst)
{

    printf ("PDK LPC Reset is invoked\n");

    g_pdkSnoopCounter=0;
    g_pdkSnoopFlag=0;

    API_RunInitializationAgent (BMCInst);
    return;
}
/*-------------------------------------------------------------------------
 * @fn PDK_PreInit
 * @brief This function is called by the core after before any initialization
 *------------------------------------------------------------------------*/
void
PDK_PreInit (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_PostInit
 * @brief This function is called by the core after after all initialization
 *------------------------------------------------------------------------*/
void
PDK_PostInit (int BMCInst)
{
	//printf("PDK_PostInit\n");

    PDK_RegisterAllFRUs(BMCInst);

	return;
}

int
PDK_PreMonitorIndividualSensor (SensorInfo_T *pSensorInfo, int BMCInst)
{
    if(0)
    {
        pSensorInfo = pSensorInfo;    /* -Wextra; Fix for unused parameter */
        BMCInst = BMCInst;
    }
#ifdef USE_PECI_TO_READ_CPU_MEMORY_SENSOR_VALUE
    MsgPkt_T MsgPkt;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U ResIndex=0, counter=0, TempX=0;
    INT8U index=0,chksumlen=0;
    FILE *fPtr;
	static INT8U recv_cpu0_tjmax=0;
	static INT8U recv_cpu1_tjmax=0;
	static INT8U recv_cpu2_tjmax=0;
	static INT8U recv_cpu3_tjmax=0;
	
    if (pBMCInfo->IpmiConfig.NodeMgrSupport)
    {	
		/*get CPU#0 TjMAX*/
		if((CPU_SEL & 0x01) != 0)
		{
			if(recv_cpu0_tjmax==0)
			{
		   		PDK_GetCPUTJMAX(BMCInst,0);
		   		recv_cpu0_tjmax = 1;
			}
		
			if(recv_cpu0_tjmax==1)
			{
		   		if(blGetMessage_cpu==1)
			 	PDK_GetCPUTJMAX(BMCInst,0);		
			}
		}

		/*get CPU#1 TjMAX*/
		if((CPU_SEL & 0x02) !=0)
		{
			
			if(recv_cpu1_tjmax==0)
			{
		   		PDK_GetCPUTJMAX(BMCInst,1);
		   		recv_cpu1_tjmax = 1;
			}
		
			if(recv_cpu1_tjmax==1)
			{
		  	 	if(blGetMessage_cpu==1)
			 	PDK_GetCPUTJMAX(BMCInst,1);		
			}	
		}

		/*get CPU#2 TjMAX*/
		if((CPU_SEL & 0x04) != 0)
		{
			
			if(recv_cpu2_tjmax==0)
			{
		   		PDK_GetCPUTJMAX(BMCInst,2);
		   		recv_cpu2_tjmax = 1;
			}

			if(recv_cpu2_tjmax==1)
			{
		   		if(blGetMessage_cpu==1)
			 	PDK_GetCPUTJMAX(BMCInst,2);		
			}
		}

		/*get CPU#3 TjMAX*/
		if((CPU_SEL & 0x08) != 0)
		{
			
			if(recv_cpu3_tjmax==0)
			{
		   		PDK_GetCPUTJMAX(BMCInst,3);
		   		recv_cpu3_tjmax = 1;
			}

			if(recv_cpu3_tjmax==1)
			{
		   		if(blGetMessage_cpu==1)
			 	PDK_GetCPUTJMAX(BMCInst,3);		
			}	
		}

		 /*make sure that we get CPU TJMAX vaue */
         if ((blGetMessage == 0) && (recv_cpu_tjmax == 1) )
        {
            /*ensure the Receive Message queue is existed*/
            fPtr = fopen(RCV_MSG_Q_01, "r");
            if (fPtr)
            {
                fclose(fPtr);
                
                memset(MsgPkt.Data, 0, MSG_PAYLOAD_SIZE);
                /* Construct the request packet for Get CPU and Memory temperature Information*/
                MsgPkt.NetFnLUN = NETFN_APP << 2;
                MsgPkt.Cmd      = CMD_SEND_MSG;
                MsgPkt.Data[0] = pBMCInfo->SMLINKIPMBCh; /*Channel Number*/
                MsgPkt.Data[1] = PNM_PECI_ME_SLAVE_ADDRESS;
                MsgPkt.Data[2] = PNM_PECI_PROXY_FNLUN;
                MsgPkt.Data[3] = (INT8U)CalculateCheckMsgSum (&MsgPkt.Data[1], 2);
                MsgPkt.Data[4] = pBMCInfo->IpmiConfig.BMCSlaveAddr;
                MsgPkt.Data[5] = 0x01;
                MsgPkt.Data[6] = CMD_PNM_OEM_PECI_PROXY_GET_CPU_AND_MEMORY_TEMPERATURE;
                MsgPkt.Data[7] = 0x57;  /* Intel Manufacturer ID !V 000157h */
                MsgPkt.Data[8] = 0x01;
                MsgPkt.Data[9] = 0x00;
                MsgPkt.Data[10] = CPU_SEL;
                MsgPkt.Data[11] = CPU_0_1_MEMORY_CHANNEL;
                MsgPkt.Data[12] = CPU_2_3_MEMORY_CHANNEL;
                /*index is pointing to checksum field*/
                index=13;
                chksumlen=index-4;
                MsgPkt.Data[19]=(INT8U)CalculateCheckMsgSum (&MsgPkt.Data[4], chksumlen);
                MsgPkt.Size     = CMD_PNM_OEM_PECI_PROXY_GET_CPU_AND_MEMORY_TEMPERATURE_LEN;
                
                /* Execute the IPMI send message command*/
                API_ExecuteCmd (&MsgPkt, BMCInst);
                
                if (CC_SUCCESS == ((SendMsgRes_T*)(MsgPkt.Data))->CompletionCode)
                {
                    blGetMessage = 1;
                    GetSendsorValCounter = 0;
                }
                else
                {
                    /*IPMI_WARNING ("%s : Failed to sent CPU and Memory temperature\n",__FUNCTION__);*/
                }
            }
        }
        else
        {
            MsgPkt.NetFnLUN = (NETFN_APP << 2) | 0x00;
            MsgPkt.Cmd = CMD_GET_MSG;
            MsgPkt.Size = 0; // no request data
            API_ExecuteCmd (&MsgPkt, BMCInst);
        
            if (CC_SUCCESS == ((GetMsgRes_T*)(MsgPkt.Data))->CompletionCode)
            {
                GetSendsorValCounter = 0;
                blGetMessage = 0;
                memset(CPU_MEMORY_SENSOR_VAL, 0, END_OF_CPU_MEMORY_LIST);
                /*Update the value of CPUs and Memory Channels*/
                if (CPU_SEL == 0)
                {
                    index = CPU_0_MEMORY_CH_0_INDEX;
                }
                else
                {
                    TempX=CPU_SEL;
                    for (counter =0, index =0; counter<4; counter++, index++)
                    {
                        if ((TempX&0x1)!=0)
                        {
                            CPU_MEMORY_SENSOR_VAL[index] = MsgPkt.Data[ResIndex+PNM_DATA_START_OFFSET+PNM_COMPLETE_CODE_LEN+PNM_INTEL_MANUFACTURER_ID_LEN];
                            ResIndex++;
                        }
                        TempX=TempX>>1;
                    }
                }
        
                if (CPU_0_1_MEMORY_CHANNEL == 0)
                {
                    index = CPU_2_MEMORY_CH_0_INDEX;
                }
                else
                {
                    TempX=CPU_0_1_MEMORY_CHANNEL;
                    for (counter =0; counter<8; counter++, index++)
                    {
                        if ((TempX&0x1)!=0)
                        {
                            CPU_MEMORY_SENSOR_VAL[index] = MsgPkt.Data[ResIndex+PNM_DATA_START_OFFSET+PNM_COMPLETE_CODE_LEN+PNM_INTEL_MANUFACTURER_ID_LEN];
                            ResIndex++;
                        }
                        TempX=TempX>>1;
                    }
                }

                if (CPU_2_3_MEMORY_CHANNEL != 0)
                {
                    TempX=CPU_2_3_MEMORY_CHANNEL;
                    for (counter =0; counter<8; counter++, index++)
                    {
                        if ((TempX&0x1)!=0)
                        {
                            CPU_MEMORY_SENSOR_VAL[index] = MsgPkt.Data[ResIndex+PNM_DATA_START_OFFSET+PNM_COMPLETE_CODE_LEN+PNM_INTEL_MANUFACTURER_ID_LEN];
                            ResIndex++;
                        }
                        TempX=TempX>>1;
                    }
                }
            }
            else
            {
                if (GetSendsorValCounter < LIMITATION_TO_GET_SENDSOR_VALUE)
                    GetSendsorValCounter++;
                else
                {
                    GetSendsorValCounter = 0;
                    blGetMessage = 0;
                }
                /*IPMI_WARNING ("%s : Failed to get CPU and Memory temperature\n", __FUNCTION__);*/
            }
        }

        switch (pSensorInfo->SensorNumber)
        {
            case CPU_0_SENSORNUM :
                pSensorInfo->SensorReading = CPU_TJMAX[CPU_0_INDEX] - CPU_MEMORY_SENSOR_VAL[CPU_0_INDEX];
                break;
        
            case CPU_1_SENSORNUM :
                pSensorInfo->SensorReading = CPU_TJMAX[CPU_1_INDEX] - CPU_MEMORY_SENSOR_VAL[CPU_1_INDEX];
                break;
        
            case CPU_2_SENSORNUM :
                pSensorInfo->SensorReading = CPU_TJMAX[CPU_2_INDEX] - CPU_MEMORY_SENSOR_VAL[CPU_2_INDEX];
                break;
        
            case CPU_3_SENSORNUM :
                pSensorInfo->SensorReading = CPU_TJMAX[CPU_3_INDEX] - CPU_MEMORY_SENSOR_VAL[CPU_3_INDEX];
                break;
        
            case CPU_0_MEMORY_CH_0_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_0_MEMORY_CH_0_INDEX];
                break;
        
            case CPU_0_MEMORY_CH_1_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_0_MEMORY_CH_1_INDEX];
                break;
        
            case CPU_0_MEMORY_CH_2_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_0_MEMORY_CH_2_INDEX];
                break;
        
            case CPU_0_MEMORY_CH_3_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_0_MEMORY_CH_3_INDEX];
                break;
        
            case CPU_1_MEMORY_CH_0_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_1_MEMORY_CH_0_INDEX];
                break;
        
            case CPU_1_MEMORY_CH_1_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_1_MEMORY_CH_1_INDEX];
                break;
        
            case CPU_1_MEMORY_CH_2_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_1_MEMORY_CH_2_INDEX];
                break;
        
            case CPU_1_MEMORY_CH_3_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_1_MEMORY_CH_3_INDEX];
                break;
        
            case CPU_2_MEMORY_CH_0_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_2_MEMORY_CH_0_INDEX];
                break;
        
            case CPU_2_MEMORY_CH_1_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_2_MEMORY_CH_1_INDEX];
                break;
        
            case CPU_2_MEMORY_CH_2_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_2_MEMORY_CH_2_INDEX];
                break;
        
            case CPU_2_MEMORY_CH_3_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_2_MEMORY_CH_3_INDEX];
                break;
        
            case CPU_3_MEMORY_CH_0_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_3_MEMORY_CH_0_INDEX];
                break;
        
            case CPU_3_MEMORY_CH_1_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_3_MEMORY_CH_1_INDEX];
                break;
        
            case CPU_3_MEMORY_CH_2_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_3_MEMORY_CH_2_INDEX];
                break;
        
            case CPU_3_MEMORY_CH_3_SENSORNUM :
                pSensorInfo->SensorReading = CPU_MEMORY_SENSOR_VAL[CPU_3_MEMORY_CH_3_INDEX];
                break;
        
        }
    }
#endif /* USE_PECI_TO_READ_CPU_MEMORY_SENSOR_VALUE */

	return 0;
}



/*---------------------------------------------------------------------------
 * @fn PDK_PreMonitorAllSensors
 *
 * @brief This function is invoked before all the sensors are read and before 
 * any event action is taken. This function should return 0 if it needs to 
 * proceed to sensor monitoring cycle otherwise return 0.
 *
 * @return  0	- if need to proceed to sensor monitor cycle.
 *			-1	- otherwise
 *---------------------------------------------------------------------------*/
int
PDK_PreMonitorAllSensors (bool bInitAgentRearmed,int BMCInst)
{
    g_bInitAgentRearmed = bInitAgentRearmed;
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*---------------------------------------------------------------------------
 * @fn PDK_PostMonitorAllSensors
 *
 * @brief This function is invoked immediately after all the sensors are read
 * and before any event action is taken. This function should return 0 if this
 * sensor monitoring cycle needs to generate events, -1 otherwise.
 *
 * @param	NumSensor  	 - Number of sensors in this monitoring cycle.
 * @param	pSensorTable - Table holding the sensor readings.
 *
 * @return  0	- if sensor sensor monitoring cycle needs to generate event.
 *			-1	- otherwise
 *---------------------------------------------------------------------------*/
int
PDK_PostMonitorAllSensors ( INT8U NumSensor, _FAR_ INT16U* pSensorTable,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	NumSensor=NumSensor;
	pSensorTable=pSensorTable;
    }

    // Periodically update airflow information
    OEM_UpdateFansAirflow();
    OEM_UpdateSystemAirflow();
    return 0;
}

/*---------------------------------------------------------------------
 * @fn PDK_PreAddSEL
 *
 * @brief This control function is called just before the MegaRAC-PM core
 * adds a SEL entry.
 *
 * @param   pSELEntry - The SEL Record that is being added.
 * @param   SelectTbl - The choice of SEL, PEF or both to which the entry
 *                      should be added
 * @param   BMCInst - BMC Instance
 *
 * @return  0x0  Post to neither PEF nor SEL
 *          0x1  Post only to SEL
 *          0x3  Post to both PEF and SEL
 *-----------------------------------------------------------------------*/
INT8U
PDK_PreAddSEL (_FAR_ INT8U* pSELEntry, INT8U SelectTbl, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pSELEntry=pSELEntry;
    }
    return SelectTbl;
}
/*---------------------------------------------------------------------
 * @fn PDK_PostAddSEL
 *
 * @brief This control function is called just after the MegaRAC-PM core
 * adds a SEL entry.
 *
 * @param   pSELEntry	- The SEL Record that is being added.
 *
 * @return  0x00	The MegaRAC PM core should not add this record.
 *			0xff	The MegaRAC PM core should add this record.
 *-----------------------------------------------------------------------*/
INT8U
PDK_PostAddSEL (_FAR_ INT8U*	pSELEntry,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pSELEntry=pSELEntry;
    }
    return 0xff;
}

/*---------------------------------------------------------------------
 * @fn PDK_PreClearSEL
 *
 * @brief This control function is called just before the MegaRAC-PM core
 * clears the SEL.
 *
 *
 * @return  0x00	The MegaRAC PM core should not clear the SEL.
 *			0xff	The MegaRAC PM core should clear the SEL.
 *-----------------------------------------------------------------------*/
INT8U
PDK_PreClearSEL ( int BMCInst )
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0xff;
}


/*---------------------------------------------------------------------
 * @fn PDK_GetSELLimit
 *
 * @brief This control function is called to get the SEL Storage percentage.
 *
 * @return  0x00		The MegaRAC PM core should not handle SEL Storage limit.
 *			SELLimit	The SEL storage Limit in Perecentage.
 *-----------------------------------------------------------------------*/
INT8U
PDK_GetSELLimit ( int BMCInst )
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*---------------------------------------------------------------------
 * @fn PDK_SELLimitExceeded
 *
 * @brief This control function is called when SEL count exceded the SEL Limit
 *		  	specifed by PDK_GetSELLimit control function.
 *
 * @return  0x00	Success.
 *			0xff	Failed.
 *-----------------------------------------------------------------------*/
INT8U
PDK_SELLimitExceeded ( int BMCInst )
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}


/*-------------------------------------------------------------------------
 * @fn PDK_PEFOEMAction
 * @brief This function is called by the Platform Event Filter to perform
 * OEM Filter actions. Perform any additional filters that are OEM specific
 * and any actions for these filters.
 * @param pEvtRecord - Event record that triggered the PEF.
 * @return 	0	- Success
 *			1 	- Failed.
 *------------------------------------------------------------------------*/
int
PDK_PEFOEMAction (SELEventRecord_T*   pEvtRecord, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pEvtRecord=pEvtRecord;
    }
    return 0;
}

/*-----------------------------------------------------------------------
 * @fn PDK_250MSTimerCB
 *
 * @brief This function is called from Chassis Timer Task.
 * 		  The frequency of this callback is equal to the CHASSIS_TIMER_INTERVAL
 * 		  macro defined for the chassis task . Default is 250 millisec.
 *
 *-----------------------------------------------------------------------*/
void
PDK_ChassisTimerCB (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;

}



/*----------------------------------------------------------------------------
 * @fn PDK_TimerTask
 *
 * @brief   This function is invoked by message handler every one second.
 *----------------------------------------------------------------------------*/
void
PDK_TimerTask (int BMCInst)
{
	u8 PostCodeBuf[MAX_SNOOP_SIZE];
	hal_t phal;
	int	rd_len=0, i;
    	if(0)
    	{
        	BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    	}
	if (g_pdkSnoopFlag == 1)
		return;
	if (g_pdkSnoopCounter < SNOOP_DELAY_TIME)
	{
		g_pdkSnoopCounter ++;
		return;
	}
	printf ("Starting to Read Current PostCode buffer...\n");
	phal.read_len = sizeof (PostCodeBuf);
	phal.pread_buf = PostCodeBuf;
	rd_len=snoop_read_current_bios_code (&phal);
	if (-1 == rd_len)
	{
		printf ("Error Reading Current PostCode buffer...\n");
	}
	else if (rd_len)
	{
		printf ("Current Post Codes are ...\n");
		for (i = 0; i < rd_len; i++)
		{
			printf ("0x%02x ", phal.pread_buf[i]);
		}
		printf ("\n");
	}
	else
		printf ("No Post Codes ...\n");
	g_pdkSnoopFlag = 1;
	return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeBMCColdReset
 * @brief This function is called by the core cold reset function
 * @return  0x00	The MegaRAC PM core will cold reset BMC
 *			-1   	The MegaRAC PM core will not cold reset BMC
 *------------------------------------------------------------------------*/
int
PDK_BeforeBMCColdReset (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeBMCColdReset
 * @brief This function is called by the core cold reset function
 * @return  0x00	The MegaRAC PM core will cold reset BMC
 *			-1   	The MegaRAC PM core will not cold reset BMC
 *------------------------------------------------------------------------*/
int
PDK_BeforeBMCWarmReset (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeInitAgent
 * @brief This function is called by the core before Initialization agent.
 * @return  0x00	The MegaRAC PM core will run init agent
 *			-1   	The MegaRAC PM core will not run init agent
 *------------------------------------------------------------------------*/
int
PDK_BeforeInitAgent (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeInitAgent
 * @brief This function is called by the core after Initialization agent
 * @param   Completion code of the last SDR operation
 * @return  0x00	Init agent ran successfully
 *			-1   	Init agent did not run successfully
 *------------------------------------------------------------------------*/
int
PDK_AfterInitAgent (INT8U StatusCode,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	StatusCode=StatusCode;
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforePowerOnChassis
 * @brief This function is called by the core before performing power on chassis
 * @return  0x00	The MegaRAC PM core will perform power on chassis
 *			-1   	The MegaRAC PM core will not perform power on chassis
 *------------------------------------------------------------------------*/
int
PDK_BeforePowerOnChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterPowerOnChassis
 * @brief This function is called by the core after performing power on chassis
 *------------------------------------------------------------------------*/
void
PDK_AfterPowerOnChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforePowerOffChassis
 * @brief This function is called by the core before performing power off chassis
 * @return  0x00	The MegaRAC PM core will perform power off chassis
 *			-1   	The MegaRAC PM core will not perform power on chassis
 *------------------------------------------------------------------------*/
int
PDK_BeforePowerOffChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterPowerOffChassis
 * @brief This function is called by the core after performing power off chassis
 *------------------------------------------------------------------------*/
void
PDK_AfterPowerOffChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeResetChassis
 * @brief This function is called by the core before performing reset chassis
 * @return  0x00	The MegaRAC PM core will perform reset the chassis
 *			-1   	The MegaRAC PM core will not perform reset on the chassis
 *------------------------------------------------------------------------*/
int
PDK_BeforeResetChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterResetChassis
 * @brief This function is called by the core after performing reset chassis
 *------------------------------------------------------------------------*/
void
PDK_AfterResetChassis (int BMCInst)
{
//	Now locking is done by the caller 
//	LOCK_BMC_SHARED_MEM();

    /* reset the m_setinprogress */
    BMC_GET_SHARED_MEM(BMCInst)->m_Sys_SetInProgress = 0x00;
    _fmemset ( &(BMC_GET_SHARED_MEM(BMCInst)->OperatingSystemName), 0x00, sizeof(OSName_T));
    /* reset the OS Name */

//	UNLOCK_BMC_SHARED_MEM();
    return;
}


/*-----------------------------------------------------------------------------
 * @fn PDK_BeforePowerCycleChassis
 * @brief This function is called by the core before performing power cycle chassis
 * @return  0x00	The MegaRAC PM core will perform power cycle on chassis
 *			-1   	The MegaRAC PM core will not perform power cycle on chassis
 *-----------------------------------------------------------------------------*/
int
PDK_BeforePowerCycleChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterPowerOnChassis
 * @brief This function is called by the core after performing power cycle chassis
 *------------------------------------------------------------------------*/
void
PDK_AfterPowerCycleChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-----------------------------------------------------------------------------
 * @fn PDK_BeforeDiagInterrupt
 * @brief This function is called by the core before performing diag interrupt
 * @return  0x00	The MegaRAC PM core will rise the diag interrupt
 *			-1   	The MegaRAC PM core will not rise the diag interrupt
 *-----------------------------------------------------------------------------*/
int
PDK_BeforeDiagInterrupt (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterDiagInterrupt
 * @brief This function is called by the core after performing rising
 * the diag interrupt
 *------------------------------------------------------------------------*/
void
PDK_AfterDiagInterrupt (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeSMI
 * @brief This function is called by the core before performing SMI interrupt
 * @return  0x00	The MegaRAC PM core will rise the SMI
 *			-1   	The MegaRAC PM core will not rise SMI
 *------------------------------------------------------------------------*/
int
PDK_BeforeSMIInterrupt (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterSMI
 * @brief This function is called by the core after performing rising
 * the SMI interrupt
 *------------------------------------------------------------------------*/
void
PDK_AfterSMIInterrupt (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_BeforeSoftOffChassis
 * @brief This function is called by the core before performing soft off chassis
 * @return  0x00	The MegaRAC PM core will perform soft shutdown
 *			-1   	The MegaRAC PM core will not perform soft shutdown
 *------------------------------------------------------------------------*/
int
PDK_BeforeSoftOffChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_AfterSoftOffChassis
 * @brief This function is called by the core after performing soft shutdown
 *------------------------------------------------------------------------*/
void
PDK_AfterSoftOffChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*---------------------------------------------------------------------
 * @fn PDK_OnACPIStateChange
 *
 * @brief This control function is called when the ACPI State Changes
 * @param   PreviousState	Previous state of the chassis
 * @param   CurrentState	Current  state of the chassis
 *-----------------------------------------------------------------------*/
// It already is present with signature and it is in
// SetACPIPwrState command in IPMDevice.c (this too has signature)
void
PDK_OnACPIStateChange (int CurrentState, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	CurrentState=CurrentState;
    }
    return;
}

/*----------------------------------------------------------------------
 * @fn PDK_TerminalCmd
 * @brief This function is called to process Terminal mode command.
 * @param   pReq - Pointer to request string.
 * @param   pRes - Pointer to response string.
 * @param   ReqLen - Request string length.
 * @param   ResLen - Response string length.
 * @param   SessionActivated - Terminal Session Activated.
 * @return  0   Command handled by PDK.
 *          -1  Command not handled by PDK.
 *----------------------------------------------------------------------*/
extern int  PDK_TerminalCmd             (INT8U* pReq, INT8U  ReqLen,
                                         INT8U* pRes, INT8U* ResLen,
                                         INT8U  SessionActivated)
{
    if(0)
    {
        pReq=pReq;  /*  -Wextra, fix for unused parameter  */
	ReqLen=ReqLen;
	pRes=pRes;
	ResLen=ResLen;
	SessionActivated=SessionActivated;
    }
    return -1;
}

/*----------------------------------------------------------------------
 * @fn PDK_GetMfgProdID
 * @brief This function is used to get Manufacture ID and Product ID
 * @param[in] BMCInst - BMC instanve number.
 * @param[out] MfgID - Pointer for Manufacture ID.
 * @param[out] ProdID - Pointer for Product ID.
 * @returns 0.
 *----------------------------------------------------------------------*/
int PDK_GetMfgProdID (INT8U* MfgID, INT16U* ProdID, int BMCInst)
{
    *ProdID = AST2500EVB_PLATFORMID;
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	MfgID=MfgID;
    }
    return 0;
}

/*----------------------------------------------------------------------
 * @fn PDK_InitSOLPort
 * @brief This function will contain any OEM specific SOL initialization
 * @param ptty_struct - Pointer to terminal attributes structure.
 *----------------------------------------------------------------------*/
void PDK_InitSOLPort (struct termios *ptty_struct, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	ptty_struct=ptty_struct;
    }
    return;
}






/*----------------------------------------------------------------------
 * @fn PDK_GetSpecificSensorReading
 * @This function can be used when internal Sensor values are to be returned or
 * sensor values needs to be faked.
 *----------------------------------------------------------------------*/
int
PDK_GetSpecificSensorReading ( INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pReq=pReq;
	ReqLen=ReqLen;
	pRes=pRes;
    }
    return 0;
}

/*----------------------------------------------------------------------
 * @fn PDK_SetCurCmdChannelInfo
 * @This function updates the PDK library with the current command
 *       channel and privelege
 *----------------------------------------------------------------------*/
void
PDK_SetCurCmdChannelInfo (INT8U Channel, INT8U Privilege, int BMCInst)
{
	/* Clear VLAN packet indication */
	g_pdkIsPktFromVLAN = FALSE;
    	if(0)
    	{
        	BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    	}
	/* BIT4 indicates that this is a VLAN packet */
	if (Channel & 0xF0)
	{
		//printf ("** VLAN Pkt**\n");
		g_pdkIsPktFromVLAN = TRUE;			
	}
	
	if (Channel == SYS_IFC_CHANNEL)
	{
		//printf ("** VLAN Pkt**\n");
		g_pdkIsPktFromVLAN = TRUE;					
	}
	
	
	/* Update Current Cmd channel information */
	g_pdkChannelInfo.ChannelNum = Channel;
	g_pdkChannelInfo.Privilege  = Privilege;
}

/*----------------------------------------------------------------------
 * @fn PDK_GetCurCmdPrivilegeInfo
 * @This function is used to get current command privilege
 * @param[in] BMCInst - BMC instance number
 * @return privilege value
 *----------------------------------------------------------------------*/

INT8U PDK_GetCurCmdPrivilegeInfo(int BMCInst)
{
    UN_USED(BMCInst);
    return g_pdkChannelInfo.Privilege;
}

/*-----------------------------------------------------------------
 * @fn PDK_SetBTSMSAttn
 *
 * @brief This function is used to Set BT SMS attention bit
 *        .
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_SetBTSMSAttn (INT8U BTIFC_NUM, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    bt_data_t	BTData;

    if(pBMCInfo->IpmiConfig.BTIfcSupport == 1)
    {
        // Set SMS attention by invoking driver call.
        BTData.btifcnum = BTIFC_NUM;
        if (BTData.btifcnum > 1)
        {
            IPMI_WARNING("PDK_SetBTSMSAttn called with unknown BTIFC_NUM: %d\n", BTIFC_NUM);
            return;
        }
        if(ioctl(pBMCInfo->btfd,SET_BT_SMS_BIT,&BTData) == -1)
        {
            TCRIT("SET_BT_SMS_BIT IOCTL call failed\n");
        }
    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_SetSMSAttn
 *
 * @brief This function is used to Set SMS attention bit
 *        .
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_SetSMSAttn (INT8U KCSIfcNum, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    kcs_data_t	KCSData;

    if(pBMCInfo->IpmiConfig.KCS1IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS2IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
    {
        // Set SMS attention by invoking driver call.
        KCSData.kcsifcnum = KCSIfcNum;
        if (KCSData.kcsifcnum > 2)
        {
            IPMI_WARNING("PDK_SetSMSAttn called with unknown KCSIfcNum: %d\n", KCSIfcNum);
            return;
        }

        if(ioctl(pBMCInfo->kcsfd[KCSIfcNum],SET_SMS_BIT,&KCSData) == -1)
        {
            TCRIT("SET_SMS_BIT IOCTL call failed\n");
        }

    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_ClearSMSAttn
 *
 * @brief This function is used to Clear SMS attention bit
 *        .
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_ClearSMSAttn (INT8U KCSIfcNum, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    kcs_data_t  KCSData;

    if(pBMCInfo->IpmiConfig.KCS1IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS2IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
    {
        // Clear SMS Attention by invoking driver call
        KCSData.kcsifcnum = KCSIfcNum;

        if (KCSData.kcsifcnum > 2)
        {
            IPMI_WARNING("PDK_ClearSMSAttn called with unknown KCSIfcNum: %d\n", KCSIfcNum);
            return;
        }

        if(ioctl(pBMCInfo->kcsfd[KCSIfcNum],CLEAR_SMS_BIT,&KCSData) == -1)
        {
            TCRIT("CLEAR_SMS_BIT IOCTL call failed\n");
        }

    }
    return;
}

/**
 * @fn PDK_SetOBF
 * @brief  This function used to Set OBF bit when event message interrupt, watchdog pretimeout interrupt occurs
 * @param KCSIfcNum - KCS Interface number 
 * @param BMCInst - BMC instance
 */
void
PDK_SetOBF (INT8U KCSIfcNum, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    kcs_data_t KCSData;

    if(pBMCInfo->IpmiConfig.KCS1IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS2IfcSupport == 1 || pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
    {
        // Set OBF by invoking driver call.
        KCSData.kcsifcnum = KCSIfcNum;
        if (KCSData.kcsifcnum > 2)
        {
            IPMI_WARNING("PDK_SetOBF called with unknown KCSIfcNum: %d\n", KCSIfcNum);
            return;
        }

        if(ioctl(pBMCInfo->kcsfd[KCSIfcNum],SET_OBF_BIT,&KCSData) == -1)
        {
            TCRIT("SET_OBF_BIT IOCTL call failed\n");
        }
    }
    return;
}

/*----------------------------------------------------------------------
 * @fn PDK_WatchdogAction
 * @brief This function is used to do custom actions on watchdog expiry
 * @param TmrAction - as per IPMI spec field
 * @param pWDTTmrMgr - pointer to internal watchdog timer mgmt struct
 * @returns nothing
 *----------------------------------------------------------------------*/
void
PDK_WatchdogAction (INT8U TmrAction, WDTTmrMgr_T* pWDTTmrMgr, int BMCInst)
{
    //Do custom actions
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	TmrAction=TmrAction;
	pWDTTmrMgr=pWDTTmrMgr;
    }
    return	;
	
}

/*
 * @fn PDK_InitSystemGUID
 * @brief This function is used to override the OEM specified GUID
 * @param SystemGUID - System GUID to be override
 * @return 0 on overriding the GUID value otherwise 1
 */
int
PDK_InitSystemGUID(INT8U* SystemGUID)
{
    /* OEM can override the GUID creation by defining their
    own methods of GUID creation here
    NOTE - After creating OEM have to return 0 instead of 1*/
    if(0)
    {
        SystemGUID=SystemGUID;  /*  -Wextra, fix for unused parameter  */
    }
    return 1;
}

/*
 * @fn PDK_InitDeviceGUID
 * @brief This function is used to override the OEM specified GUID
 * @param DeviceGUID - Device GUID to be override
 * @return 0 on overriding the GUID value otherwise 1
 */
int
PDK_InitDeviceGUID(INT8U* DeviceGUID)
{
    /* OEM can override the GUID creation by defining their
    own methods of GUID creation here
    NOTE - After creating OEM have to return 0 instead of 1*/
    if(0)
    {
        DeviceGUID=DeviceGUID;  /*  -Wextra, fix for unused parameter  */
    }
    return 1;
}

/*-----------------------------------------------------------------
 * @fn PDK_LoadOEMSensorDefault
 *
 * @brief This function is used to Load any OEM Specific
 *        Sensor initialization
 *        .
 *
 * @param  Sensor Information 
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_LoadOEMSensorDefault (SensorInfo_T* pSensorInfo, int BMCInst)
{
    /* Load Default Sensor Readings */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pSensorInfo=pSensorInfo;
    }
    return ;
}

/*-----------------------------------------------------------------
 * @fn PDK_SetLastPowerEvent
 *
 * @brief This callback function is set the Last Power Event 
 *        for chassis status command.
 *
 * @param  None
 *
 * @return   None.
 *-----------------------------------------------------------------*/
void PDK_SetLastPowerEvent (INT8U Event, int BMCInst)
{
	/* Set Last Power Event  */
    gPDKLastPowerEvent = Event;
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
	
}


/*-----------------------------------------------------------------
 * @fn PDK_GetLastPowerEvent
 *
 * @brief This callback function is get the Last Power Event 
 *        for chassis status command.
 *
 * @param  None
 *
 * @return   None.
 *-----------------------------------------------------------------*/
INT8U PDK_GetLastPowerEvent (int BMCInst)
{
    /* Get the Last Power Event Status  */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return gPDKLastPowerEvent;
}





/*-----------------------------------------------------------------
 * @fn PDK_AppendOEMPETData
 *
 * @brief This hook is used to append any OEM specific 
 *        PET Data.
 *
 * @param  None
 *
 * @return   None.
 *-----------------------------------------------------------------*/
int PDK_AppendOEMPETData (VarBindings_T* pVarBindings, int Length, INT8U *AlertStr,int BMCInst)
{
	OemPETField_T *OemField =(OemPETField_T *) pVarBindings->Oem;
    	if(0)
    	{
        	BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    	}
	if(0!= AlertStr[0] )
		{
			OemField->Type =0x80;
			OemField->Length = 0x40 | MAX_ALERT_STRING_LENGTH; //Language Code and String Length
			OemField->RecType= 0x01;  //Alert  String Type
			memcpy(OemField->RecData,AlertStr,MAX_ALERT_STRING_LENGTH);
	return (Length - MAX_OEM_TRAP_FIELDS) + (3 + MAX_ALERT_STRING_LENGTH);
		}else
		{
			/* NO Oem Field */
			OemField->Type =0xC1;
	return (Length - MAX_OEM_TRAP_FIELDS) + 1;
		}

		
}

void PDK_GetSelfTestResults(INT8U* SelfTestResult, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	SelfTestResult=SelfTestResult;
    }
    return;
}
void PDK_ProcessOEMRecord(INT8U* OEMRec, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	OEMRec=OEMRec;
    }
    return;
}

void PDK_AfterSDRInit(INT8U BMCInst)
{

    /* How to Access All SDR Records? */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
#if 0
    int i;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SDRRecHdr_T*      pSDRRecord;
    pSDRRecord = SDR_GetFirstSDRRec (BMCInst);
    for (i=0; i < pBMCInfo->SDRConfig.SDRRAM->NumRecords && pSDRRecord != NULL; i++)
    {
        printf ("SDR TYPE : %x\n", pSDRRecord->Type);
        switch (pSDRRecord->Type)
        {
        case FULL_SDR_REC:
            break;
        case COMPACT_SDR_REC:
            break;
        default:
            break;
        }
        pSDRRecord = SDR_GetNextSDRRec (pSDRRecord,BMCInst);
    }
#endif
    return;
}


//<<KAMAL>>Added to support Sensor Averaging ../

#define AVERAGING_SIZE					30

int PDK_SensorAverage(INT8U SensorNum, INT8U OwnerLUN, INT16U* pSensorReading, INT8U* pReadFlags, int BMCInst)
{
    static INT16U	AverageBuf [AVERAGING_SIZE];
    static int		AverageCount = 0;
    static int		InitDone	   = 0;
    SensorInfo_T*	pSensorInfo    = NULL;
    int 			AverageIndex   = 0;
    int 			i   		   = 0;
    INT16U			AverageVal	   = 0;

    pSensorInfo = API_GetSensorInfo (SensorNum, OwnerLUN, BMCInst);
    /* Check if we received valid sensor information */
    if (pSensorInfo == NULL) 
    {
        	printf ("Unable to get SensorInfo \n");
        	return 0;  
    }		

	/* Averaging index */
	AverageIndex = AverageCount % AVERAGING_SIZE;
	
	/* Set Sensor state to be update in progress unless
	 * we receive all  AVERAGING_SIZE values */
	if (0 == InitDone) 
	{ 
		/* Settting update in progress */
		/* Mutex lock already acquired in Sensor monitor task before invoking the PDK */
		/* coverity[missing_lock : FALSE] */
		pSensorInfo->EventFlags |= BIT1;
		*pReadFlags = 1; /* No event generation */
		
		/* Checking if we have enough to do averaging */
		if (AverageIndex == (AVERAGING_SIZE -1))
		{
			InitDone = 1;
		}
	}
	else
	{
		/* Clearing update in progress */
		pSensorInfo->EventFlags &= ~BIT1;
	}
	
	/* Update averaging buffer */	
	AverageBuf [AverageIndex ] = 	*pSensorReading;
	
	AverageCount++;
	
	if (AverageCount == AVERAGING_SIZE) { AverageCount = 0; }
	
	for (i = 0; i <  AVERAGING_SIZE; i++)
	{
		AverageVal += AverageBuf [i];
	}

	if (0 == InitDone)
	{
		*pSensorReading = 0;
		*pReadFlags 	= 1;				
	}
	else
	{
		*pSensorReading = AverageVal / AVERAGING_SIZE;
		*pReadFlags = 0;
	}
	//printf ("Averaged value - %x\n", *pSensorReading);
		
	/* Averaging done */
	return 0;	
}

int  PDK_SMCDelay(INT8U SlaveAddr, int BMCInst)
{
/*
	switch(SlaveAddr)
	{
	case  MMC_ADDR :
		usleep(500000);
		break;
	default :
		break;
	}
*/
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	SlaveAddr=SlaveAddr;
    }
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_PnmOemGetReading
 *
 * @brief This hook is used to get the sensor value of PNM power
 * consumption, inlet air temperature.
 *
 * @param Readingtype - Tartget Sensor type.
 * @param DomainID - Domain Id.
 * @param SensorReading - Sensor value.
 * 
 * @return   The status of Sensor reading.
 *-----------------------------------------------------------------*/
INT8U PDK_PnmOemGetReading (INT8U Readingtype, INT8U DomainID, INT16U *SensorReading,int BMCInst)
{
    /* Get the Sensor value  */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Readingtype=Readingtype;
	DomainID=DomainID;
	SensorReading=SensorReading;
    }
    return 0;
}


/*-----------------------------------------------------------------
 * @fn PDK_PnmOemMePowerStateChange
 *
 * @brief This control function is called when the ME power state
 *        change.
 *
 * @param PowerState - ME power state.
 *
 * @return   None.
 *-----------------------------------------------------------------*/
void PDK_PnmOemMePowerStateChange (INT8U PowerState,int BMCInst)
{
    /* Get the ME power state  */
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	PowerState=PowerState;
    }
    return;
}
/*
 * @fn PDK_BiosIPSource
 * @brief This function is used to override Bios IP Source actions
 * @ChannelNumber - channel number
 * @return 0 on overriding - otherwise 1
 */
int
PDK_BiosIPSource(INT8U ChannelNumber )
{
    /* OEM can override the Bios IP address by defining their
    own methods here
    NOTE - After creating OEM have to return 0 instead of 1*/
    if(0)
    {
        ChannelNumber=ChannelNumber;  /*  -Wextra, fix for unused parameter  */
    }
    return 1;
}

/*
 * @fn PDK_BMCOtherSourceIP
 * @brief This function is used to override bmc other source IP actions
 * @ChannelNumber - channel number
 * @return 0 on overriding - otherwise 1
 */
int
PDK_BMCOtherSourceIP(INT8U ChannelNumber )
{
    /* OEM can override the bmc other source IP actions by defining their
    own methods here
    NOTE - After creating OEM have to return 0 instead of 1*/
    if(0)
    {
        ChannelNumber=ChannelNumber;  /*  -Wextra, fix for unused parameter  */
    }
    return 1;
}

/*---------------------------------------------------------------------------
 * @fn PDK_SetPreserveStatus
 *
 * @brief This function is invoked to set the preserve configuration status
 *---------------------------------------------------------------------------*/
int
PDK_SetPreserveStatus (int Selector, int Status, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Status=Status;
	Selector=Selector;
    }
    return 0;
}

/*---------------------------------------------------------------------------
 * @fn PDK_GetPreserveStatus
 *
 * @brief This function is invoked to get the preserve configuration status
 *---------------------------------------------------------------------------*/
int
PDK_GetPreserveStatus (int Selector, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Selector=Selector;
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_RMCPLoginAudit
 * @brief This function is called by RMCP to add AuditLog events
 * @return   0	OEM SEL Record Added successfully.
 *			-1  OEM SEL Record adding failed
 *------------------------------------------------------------------------*/
int PDK_RMCPLoginAudit (INT8U EvtType, INT8U UserId, INT8U* UserName, INT8U *ipaddr, int BMCInst)
	{
    		if(0)
    		{
		        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
			EvtType=EvtType;
			UserId=UserId;
			UserName=UserName;
			ipaddr=ipaddr;
    		}
	        return -1;
	}

/*-----------------------------------------------------------------
 * @fn PDK_PreOpState
 *
 * @brief This control is called by Operation State Machine before
 *        the initialization.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_PreOpState(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_PostOpState
 *
 * @brief This control is called by Operation State Machine after
 *        all initialization.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_PostOpState(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_IsInsertionCriteriaMet
 *
 * @brief This is provided to implement the platform specific
 *        insertion criteria for Operational State Machine.
 *
 * @return None.
 *-----------------------------------------------------------------*/
BOOL
PDK_IsInsertionCriteriaMet(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return TRUE;
}

/*-----------------------------------------------------------------
 * @fn PDK_AggregateThermal
 *
 * @brief This function is invoked to aggregate thermal sensor
 *        readings to represent the overall health of blade.
 *
 * @param pSensorInfo - The SensorInfo structure.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_AggregateThermal(SensorInfo_T *pSensorInfo, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pSensorInfo=pSensorInfo;
    }
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_AggregateFault
 *
 * @brief This function is invoked to aggregate specified sensor
 *        readings to represent the overall health of blade.
 *
 * @param pSensorInfo - The SensorInfo structure.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_AggregateFault(SensorInfo_T *pSensorInfo, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pSensorInfo=pSensorInfo;
    }
    return;
}

/* The below PDK Hooks are needed for defining OEM related
     IPMI Configurations that needs to be preserved in NVRAM.
     The code in commented part shows the sample implementation*/
#if 0
typedef struct
{
    INT8U OEMConfig1;
}
OEMConfig_T;

OEMConfig_T OemConfig;
static INT32U OEMConfAddr=0;

#define OEM_CONFIG_FILE(Instance,filename)    \
sprintf(filename,"%s%d/%s",NV_DIR_PATH,Instance,"oemcfg.ini");
#endif

/**
*@fn PDK_OEMIPMIConfigs
*@brief This function is invoked to Initialize OEM Configs from NVRAM to RAM
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int PDK_OEMIPMIConfigs(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
#if 0
    char Filename[MAXFILESIZE];
    INT8U (*ParHandlr)(char *,OEMConfig_T *);
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    OEM_CONFIG_FILE(BMCInst,Filename);

    ParHandlr = dlsym(pBMCInfo->dlpar_handle,"OEMConfig_LoadFile");

    if(ParHandlr(Filename,&OemConfig) == -1)
    {
        IPMI_ERROR("Error in accessing %s \n",Filename);
        return -1;
    }
#endif
    return 0;
}

/**
*@fn PDK_OEMIPMIConfigdat
*@brief This function helps to write OEM IPMI Configurations to IPMIConfig.dat file
*@param Filename - Pointer to Filename (IPMIConfig.dat)
*@param IPMIConfigSize - Pointer to size of IPMI Configurations
*@param BMCInst - BMC Instance
*@return Returns '0' on success
*            Returns '-1' on failure
*/
int PDK_OEMIPMIConfigdat(char *Filename,INT32U *IPMIConfigSize,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	IPMIConfigSize=IPMIConfigSize;
	Filename=Filename;
    }
#if 0
    OEMConfAddr = *IPMIConfigSize+sizeof(INT8U);
    if(ReadWriteNVR(Filename,(INT8U*)&OemConfig.OEMConfig1,OEMConfAddr,sizeof(OEMConfig_T),WRITE_NVR) == -1)
    {
        return -1;
    }
    *IPMIConfigSize += sizeof(OEMConfig_T);
#endif
    return 0;
}

/**
*@fn PDK_LoadOEMConfigdat
*@brief Load OEM IPMI Configurations from IPMIConfig.dat file
*@param Filename - Pointer to Filename (IPMIConfig.dat)
*@param BMCInst - BMC Instance
*@return Returns '0' on success
*            Returns '-1' on failure
*/
int PDK_LoadOEMConfigdat(char *Filename,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Filename=Filename;
    }
#if 0
    if(ReadWriteNVR (Filename, (INT8U*)&OemConfig.OEMConfig1, OEMConfAddr, sizeof(OEMConfig_T), READ_NVR) == -1)
    {
        return -1;
    }
#endif
    return 0;
}

/**
*@fn PDK_FlushOEMConfigs
*@brief Writes to oemspecific ini files
*@param BMCInst - BMC Instance
*@return Returns '0' on sucess
*            Returns '-1' on failure
*/
int PDK_FlushOEMConfigs(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
#if 0
    char Filename[MAXFILESIZE];
    int (*ParHandlr)(char *,OEMConfig_T *);
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    OEM_CONFIG_FILE(BMCInst,Filename);
    if(!IsCardInFlashMode())
    {
        ParHandlr = dlsym(pBMCInfo->dlpar_handle,"OEMConfig_SaveFile");
        
        if(ParHandlr(Filename,&OemConfig) == -1)
        {
            IPMI_ERROR("Error in flushing to  %s \n",Filename);
            return -1;
        }
    }
    else
    {
        IPMI_WARNING("Card is in flash mode..skipping any NV updates.If you are in NFS make sure the file /var/flasher.initcomplete is removed\n");
    }
#endif
    return 0;

}

/**
*@fn PDK_GetOEMConfigSize
*@brief Retrives OEM IPMI Configuration size
*@param IpmiConfigSize - Pointer which holds IPMI Configuration size
*@param BMCInst - BMC Instance
*@return Returns '0' always
*/
int PDK_GetOEMConfigSize(INT32U *IpmiConfigSize,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	IpmiConfigSize=IpmiConfigSize;
    }
#if 0
    *IpmiConfigSize += sizeof(OEMConfig_T);
#endif
    return 0;
}


/**
*@fn PDK_SetSSIFAlert
*@brief This function is used to set SSIF HW alert pin.
*@param Enable - Enable/Disable Alert
*@param BMCInst - BMC Instance
*@return Returns '0' on sucess
*            Returns '-1' on failure
*/
int PDK_SetSSIFAlert(BOOL Enable, int BMCInst)
{
	u8 val;
	hal_t hal;
	if(0)
    	{
            BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    	}
	/* Set GPIO_SSIF_ALERT_N Direction Output */
	val = 1;
	hal.func = HAL_SET_GPIO_DIR;
	hal.pwrite_buf = &val;
	hal.gpio.pin = GPIO_SSIF_ALERT_N;
	gpio_write(&hal);
	
	/* Set GPIO_SSIF_ALERT_N Data */
	if (Enable) val = 0;
	else val = 1;
	hal.func = HAL_DEVICE_WRITE;
	hal.pwrite_buf = &val;
	hal.gpio.pin = GPIO_SSIF_ALERT_N;
	gpio_write(&hal);
	
    TDBG("PDK_SetSSIFAlert GPIO %d to %s %d\n", GPIO_SSIF_ALERT_N, (Enable?"Enable":"Disable"),val);
    
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_BlockCmdTillBoot
 * @brief This hook is used to block specific command execution till BMC boot finishes
 * @param NetFn - Net function in request
 * @param Cmd - Command passed in request
 * @param BMCInst - BMC instance
 *
 * @return 0    - if the requested command needs to be blocked
 *         -1   - if the requested command can be allowed
 *-----------------------------------------------------------------*/
int PDK_BlockCmdTillBoot (INT8U NetFn, INT8U Cmd, int BMCInst)
{
	int ret = -1, i = 0;
	typedef struct {
		INT8U NetFn;
		INT8U Cmd;
	} reqCmd;

	/* Block all media related SET commands till boot */
	reqCmd blockCmdList[] = {
				{0x32, 0x6A},
				{0x32, 0x9F},
				{0x32, 0xA0},
				{0x32, 0xB8},
				{0x32, 0xC1},
				{0x32, 0xCB},
				{0x32, 0xD7},
				{0x32, 0xD9}
				};

	/* Perform the Net Function and Command check only if both Redfish and Host Inventory Support is enabled */
	if(g_corefeatures.redfish_support && g_corefeatures.hhm_hostinterface_support)
	{
		for( i = 0; i < (int)(sizeof(blockCmdList)/sizeof(blockCmdList[0])); i++ )
		{
			if( (blockCmdList[i].NetFn == NetFn) && (blockCmdList[i].Cmd == Cmd) )
			{
				TDBG("PDK_BlockCmdTillBoot match found with NetFn 0x%02x and Cmd 0x%02x, if BMC is not booted command will be blocked\n", NetFn, Cmd);
				ret = 0;
			}
		}
	}

	UN_USED(BMCInst);

	return ret;
}

/*-----------------------------------------------------------------
 * @fn PDK_PasswordValidation
 * @brief This hook is used to Validate the password
 * @Params :
 * Password [in] : Holds the password to Validate.
 * PasswordLen [in] : Holds the length of the password
 * UserID [in] : User ID of the requested user
 * @return  0 on success
 *          -1 on failure
 *  *-----------------------------------------------------------------*/
int PDK_PasswordValidation (INT8U *Password, INT8U PasswordLen, INT8U UserID)
{
    UN_USED(PasswordLen);
    UN_USED(UserID);

    /*This Hook can be used to Validate the password as required by the OEM,
      OEM can add their own conditions to check whether the password is according to their specification standards.
      If the password matches the requirement then return success else return failure.
    */

    // Check whether password length is minimum of 8 characters.
    if(strlen((const char *)Password) < MIN_PASSWORD_LEN )
    {
        TCRIT("PDK_PasswordValidation : Password length doesn't match minimum password length requirement.");
        return -1;
    }

    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_ValidOEMComponent
 *
 * @brief   Checking OEM component related functions
 * @param - ComponentID, component ID to be validated
 *
 * @return 0
 *-----------------------------------------------------------------*/
int PDK_ValidOEMComponent(int ComponentID)
{
    UN_USED(ComponentID);
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_AddOEMComponent
 * 
 * 
 *
 * @brief   Add OEM component to the Flash table
 * @param - ComponentFlashTbl- Flash table to be updated
 * 			ValidComponentIDTbl- table of valid components
 * 			HPMConfTargetCap-hpm capabilities structure
 *@return 0
 *-----------------------------------------------------------------*/
void PDK_AddOEMComponent(void * ComponentFlashTbl,INT8U* ValidComponentIDTbl,void* HPMConfTargetCap)
{
    UN_USED(ComponentFlashTbl);
    UN_USED(ValidComponentIDTbl);
    UN_USED(HPMConfTargetCap);
    return;
}

/*-----------------------------------------------------------------
 * @fn PDK_OEMEraseCopyFlash
 * 
 * 
 *
 * @brief   erasecopyflash command for oem components
 * @param - FirmID- holds the firmware ID
 * 			ComponentID- holds the component ID
 * 			FlashPart- FlashPart
 * 			WriteMemOff-WriteMemOff
 * 			Section_Flashing-Section_Flashing
 * 			Flashoffset-Flashoffset
 * 			Sizetocpy-Sizetocpy
 * 			BMCInst- BMCInst
 * 			
 *@return 0
 *-----------------------------------------------------------------*/
int PDK_OEMEraseCopyFlash(INT32U FirmID,INT8U ComponentID, INT8U FlashPart, INT32U WriteMemOff, INT32U Section_Flashing,INT32U Flashoffset, INT32U Sizetocpy, int BMCInst)
{
    UN_USED(FirmID);
    UN_USED(ComponentID);
    UN_USED(FlashPart);
    UN_USED(WriteMemOff);
    UN_USED(Section_Flashing);
    UN_USED(Flashoffset);
    UN_USED(Sizetocpy);
    UN_USED(BMCInst);
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_OEMVerifyFlash
 * 
 * 
 *
 * @brief   erasecopyflash command for oem components
 * @param - FirmID-holds the firmware ID
 * 			ComponentID- holds the component ID
 * 			FlashPart- FlashPart
 * 			VWriteMemOff-WriteMemOff
 * 			VFlashoffset-Flashoffset
 * 			Sizetoverify-Sizetoverify
 * 			BMCInst- BMCInst
 * 			
 *@return 0
 *-----------------------------------------------------------------*/
int PDK_OEMVerifyFlash(INT32U FirmID,INT8U ComponentID, INT8U FlashPart, INT32U VWriteMemOff,INT32U VFlashoffset, INT32U Sizetoverify, int BMCInst)
{
    UN_USED(FirmID);
    UN_USED(ComponentID);
    UN_USED(FlashPart);
    UN_USED(VWriteMemOff);
    UN_USED(VFlashoffset);
    UN_USED(Sizetoverify);
    UN_USED(BMCInst);
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_ActivateBIOSFlashAccess
 *
 * @brief  This function is used to Activate BIOS access to BMC for Flash
 * @params  ComponentID - Bios component ID
 * @return 	0
 *-------------------------------------------------------------------------*/
int PDK_ActivateBIOSFlashAccess(int ComponentID)
{
    UN_USED(ComponentID);
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn int PDK_DeActivateBIOSFlashAccess
 *
 * @brief  This function is used to De-Activate BIOS access to BMC for Flash
 *  
 * @params   PreviousSPIAccess - previous SPI access settings
 * @return 0
 *-------------------------------------------------------------------------*/
void PDK_DeActivateBIOSFlashAccess(int PreviousSPIAccess)
{
    UN_USED(PreviousSPIAccess);
    return;
}

/*-------------------------------------------------------------------------
 * @fn int PDK_OEMinituploadIPMC
 *
 * @brief  This function is used to perform initialization before upload of image 
 *  
 * @params   FirmID - holds the firmware ID
 * 			 CompID - holds the component ID
 * 			 HPMCmdStatus - HPMCmdStatus
 * 			 InitUpFwThreadState - InitUpFwThreadState
 * 			 BMCInst - BMCInst
 * 
 * @return 0
 *-------------------------------------------------------------------------*/
int PDK_OEMinituploadIPMC(INT32U* FirmID,INT8U CompID, void * HPMCmdStatus, int* InitUpFwThreadState ,void *ComponentFlashTbl,int BMCInst)
{
    UN_USED(FirmID);
    UN_USED(CompID);
    UN_USED(HPMCmdStatus);
    UN_USED(InitUpFwThreadState);
    UN_USED(ComponentFlashTbl);
    UN_USED(BMCInst);
    return 0;
}

/*-------------------------------------------------------------------------
 * @fn int PDK_OEMHandleFirmwareBlock
 *
 * @brief  This function is used to handle the firmware block uploaded
 *  
 * @params   FirmID - holds the firmware ID
 * 			 CompID - holds the component ID
 * 			 Len - Len
 * 			 Data - Data
 * 			 TotalDataLen - TotalDataLen
 * 			 BMCInst - BMCInst
 * @return 0
 *-------------------------------------------------------------------------*/
int PDK_OEMHandleFirmwareBlock(INT32U FirmID,INT8U CompID, INT32U  Len, INT8U* Data ,INT32U* TotalDataLen,int BMCInst)
{ 
    UN_USED(FirmID);
    UN_USED(CompID);
    UN_USED(Len);
    UN_USED(Data);
    UN_USED(TotalDataLen);
    UN_USED(BMCInst);
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_GetNVDIMMEnrgySensorNum
 * @brief This hook is used to get NVDIMM Energy sensor number
 * @param BMCInst - BMC instance
 * 
 * @return Returns 0 on success
 *         Returns -1 on failure
 *-----------------------------------------------------------------*/
int PDK_GetNVDIMMEnrgySensorNum (INT8U *SensorNum, int BMCInst)
{
	UN_USED(SensorNum);
	UN_USED(BMCInst);
	return 0;
}

/*-------------------------------------------------------------------------
 * @fn int PDK_SelectUSBPortForRISStart
 * 
 * @brief  This function is used to update USB Port Value for Media Redirection
 * @params      pPort - USB Port Value (USB PORT A - 0, USB PORT B - 1)
 *              pReserved - reserved for future use
 *              BMCInst - BMC Instance
 *-------------------------------------------------------------------------*/
void PDK_SelectUSBPortForRISStart(int *pPort, int *pReserved, int BMCInst)
{
	UN_USED(pReserved);
	UN_USED(BMCInst);
	*pPort = 0;
	return;
}

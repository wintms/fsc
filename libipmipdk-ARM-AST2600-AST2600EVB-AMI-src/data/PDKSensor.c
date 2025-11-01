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
 * pdksensor.c
 * Hooks that are invoked at different points sensor monitoring
 *
 *  Author:Pavithra S (pavithras@amiindia.co.in)
 ******************************************************************/


#include "PDKSensor.h"
#include "IPMIConf.h"

#include "OEMSensor.h"
#include "OEMFAN.h"


static SensorHooks_T pdk_sensor_hooks []=
{
    {FAN1_FRONT_RPM_SENSOR, PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN1_REAR_RPM_SENSOR,  PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN2_FRONT_RPM_SENSOR, PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN2_REAR_RPM_SENSOR,  PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN3_FRONT_RPM_SENSOR, PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN3_REAR_RPM_SENSOR,  PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN4_FRONT_RPM_SENSOR, PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN4_REAR_RPM_SENSOR,  PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN5_FRONT_RPM_SENSOR, PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
    {FAN5_REAR_RPM_SENSOR,  PDK_PreMonitorFanSensors, PDK_PostMonitorFanSensors},
};

int 
PDK_RegisterSensorHooks(int BMCInst)
{
	_FAR_ SensorSharedMem_T*	pSMSharedMem; 
	_FAR_ SensorInfo_T* 		pSensorInfo;
	_FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	unsigned int i=0;
	
	//pSMSharedMem = (_FAR_ SensorSharedMem_T*)&g_SensorSharedMem;
	pSMSharedMem = (_FAR_ SensorSharedMem_T*)&pBMCInfo->SensorSharedMem;
	if(pSMSharedMem == NULL)
	{
		IPMI_WARNING("Sensor shared mem is null!!\n");
		return -1;
	}
	
	for (i=0; i<sizeof(pdk_sensor_hooks)/sizeof(pdk_sensor_hooks[0]); i++)
	{
		
		pSensorInfo  = (SensorInfo_T*)&pSMSharedMem->SensorInfo[pdk_sensor_hooks[i].SensorNum];
		if (pSensorInfo == NULL ) {  return -1; }
		if(pdk_sensor_hooks[i].pPreMonitor != NULL)
		{
			pSensorInfo->pPreMonitor  = pdk_sensor_hooks[i].pPreMonitor;
		}
		if(pdk_sensor_hooks[i].pPostMonitor != NULL)
		{
			pSensorInfo->pPostMonitor = pdk_sensor_hooks[i].pPostMonitor;
		}
	}
	return 0;
}

/*---------------------------------------------------------------------------
 * @fn PDK_PreMonitorSensor
 *
 * @brief This function is invoked before a sensor is monitored. This
 * function can be used in place of the default sensor monitoring code.
 * This function should return 0 if the default sensor monitoring needs
 * to be run, -1 otherwise.
 *
 * @param	SensorNum		- The sensor number that is being monitored.
 * @param	pSensorReading  - Pointer to the buffer to hold the sensor reading.
 * @param	pReadFlags  	- Pointer to the flag holding the read status.
 *
 * @return  0	- if default sensor monitoring needs to run.
 *			-1	- otherwise
 *---------------------------------------------------------------------------*/
int
PDK_PreMonitorSensor_1 (void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
	//SensorInfo_T*  		pSensorInfo = pSenInfo;
	//printf("!!!!!---entering the pre monitor sensor1 for sen[%x]\n",pSensorInfo->SensorNumber);
	if(0)
	{
	    pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
	    pReadFlags=pReadFlags;
	    BMCInst=BMCInst;
	}
	return 0;
}

int
PDK_PreMonitorSensor_2 (void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
	//SensorInfo_T*  		pSensorInfo = pSenInfo;
	//printf("---entering the pre monitor sensor 2 for sen[%x]---\n",pSensorInfo->SensorNumber);
        if(0)
        {
            pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
            pReadFlags=pReadFlags;
            BMCInst=BMCInst;
        }
	return 0;
}

int
PDK_PreMonitorSensor_3 (void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
    //SensorInfo_T*  		pSensorInfo = pSenInfo;
	//printf("---entering the pre monitor sensor 3  for sen[%x]---\n",pSensorInfo->SensorNumber);
        if(0)
        {
            pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
            pReadFlags=pReadFlags;
            BMCInst=BMCInst;
        }
	return 0;
}

/*---------------------------------------------------------------------------
 * @fn PDK_PostMonitorSensor
 *
 * @brief This function is invoked immediately after the sensor monitoring
 * code is run. This function can be used to override the values returned,
 * or perform certain actions based on the sensor reading etc. This function
 * should return 0 if this sensor reading needs to be considered for this
 * cycle, -1 otherwise.
 *
 * @param	SensorNum		- The sensor number that is being monitored.
 * @param	pSensorReading	- Pointer to the buffer to hold the sensor reading.
 * @param	pReadFlags		- Pointer to the flag holding the read status.
 *
 * @return	0	- if sensor reading is to be considered for this cycle.
 *			-1	- otherwise
 *---------------------------------------------------------------------------*/
int
PDK_PostMonitorSensor_1 ( void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
    //SensorInfo_T*  		pSensorInfo = pSenInfo;
	//printf("!!!!!---entering the POST monitor sensor1 for sen[%x]\n",pSensorInfo->SensorNumber);
        if(0)
        {
            pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
            pReadFlags=pReadFlags;
            BMCInst=BMCInst;
        }
	return 0;
}

int
PDK_PostMonitorSensor_2 (void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
	//SensorInfo_T*		   pSensorInfo = pSenInfo;
	//printf("!!!!!---entering the POST monitor sensor1 for sen[%x]\n",pSensorInfo->SensorNumber);
        if(0)
        {
            pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
            pReadFlags=pReadFlags;
            BMCInst=BMCInst;
        }
	return 0;
}
int
PDK_PostMonitorSensor_3 (void*  pSenInfo,INT8U *pReadFlags,int BMCInst)
{
	//SensorInfo_T*		   pSensorInfo = pSenInfo;
	//printf("!!!!!---entering the POST monitor sensor1 for sen[%x]\n",pSensorInfo->SensorNumber);
        if(0)
        {
            pSenInfo=pSenInfo;  /*  -Wextra, fix for unused parameters  */
            pReadFlags=pReadFlags;
            BMCInst=BMCInst;
        }
	return 0;
}

/**
 * @fn PDK_PreMonitorFanSensors
 *
 * @brief This function is invoked before a fan sensor is monitored. This
 * function can be used in place of the default sensor monitoring code.
 * This function should return 0 if the default sensor monitoring needs
 * to be run, -1 otherwise.
 *
 * @param pSenInfo      - Pointer to the sensor info structure.
 * @param pReadFlags    - Pointer to the flag holding the read status.
 * @param BMCInst       - The BMC instance.
 *
 * @return  0   - if default sensor monitoring needs to run.
 *          -1  - otherwise
 */
int PDK_PreMonitorFanSensors (void* pSenInfo, INT8U *pReadFlags, int BMCInst)
{
    /*  -Wextra, fix for unused parameters  */
    if (0)
    {
        pSenInfo=pSenInfo;
        pReadFlags=pReadFlags;
        BMCInst=BMCInst;
    }
    return 0;
}

/**
 * @fn PDK_PostMonitorFanSensors
 *
 * @brief This function is invoked immediately after the fan sensor monitoring
 * code is run. This function can be used to override the values returned,
 * or perform certain actions based on the sensor reading etc. This function
 * should return 0 if this sensor reading needs to be considered for this
 * cycle, -1 otherwise.
 *
 * @param pSenInfo      - Pointer to the sensor info structure.
 * @param pReadFlags    - Pointer to the flag holding the read status.
 * @param BMCInst       - The BMC instance.
 *
 * @return  0   - if sensor reading is to be considered for this cycle.
 *          -1  - otherwise
 */
int PDK_PostMonitorFanSensors (void* pSenInfo, INT8U *pReadFlags, int BMCInst)
{
    /*  -Wextra, fix for unused parameters  */
    if (0)
    {
        pReadFlags=pReadFlags;
        BMCInst=BMCInst;
    }

    INT8U fan_id;
    INT8U fan_rotor;
    INT8U fan_rpm_raw = 0;
    INT16U fan_rpm = 0;
    int ret = -1;
    SensorInfo_T* pSensorInfo = pSenInfo;

    switch(pSensorInfo->SensorNumber)
    {
        case FAN1_FRONT_RPM_SENSOR:
            fan_id = SYS_FAN1;
            fan_rotor = FAN_ROTOR_FRONT;
            break;
        case FAN1_REAR_RPM_SENSOR:
            fan_id = SYS_FAN1;
            fan_rotor = FAN_ROTOR_REAR;
            break;
        case FAN2_FRONT_RPM_SENSOR:
            fan_id = SYS_FAN2;
            fan_rotor = FAN_ROTOR_FRONT;
            break;
        case FAN2_REAR_RPM_SENSOR:
            fan_id = SYS_FAN2;
            fan_rotor = FAN_ROTOR_REAR;
            break;
        case FAN3_FRONT_RPM_SENSOR:
            fan_id = SYS_FAN3;
            fan_rotor = FAN_ROTOR_FRONT;
            break;
        case FAN3_REAR_RPM_SENSOR:
            fan_id = SYS_FAN3;
            fan_rotor = FAN_ROTOR_REAR;
            break;
        case FAN4_FRONT_RPM_SENSOR:
            fan_id = SYS_FAN4;
            fan_rotor = FAN_ROTOR_FRONT;
            break;
        case FAN4_REAR_RPM_SENSOR:
            fan_id = SYS_FAN4;
            fan_rotor = FAN_ROTOR_REAR;
            break;
        case FAN5_FRONT_RPM_SENSOR:
            fan_id = SYS_FAN5;
            fan_rotor = FAN_ROTOR_FRONT;
            break;
        case FAN5_REAR_RPM_SENSOR:
            fan_id = SYS_FAN5;
            fan_rotor = FAN_ROTOR_REAR;
            break;
        default:
            return 0;
    }

    ret = OEM_GetFanTrayRPM(fan_id, fan_rotor, &fan_rpm);
    if (ret == 0)
    {
        fan_rpm_raw = (INT8U)(fan_rpm / FAN_RPM_MULTIPLIER);
        pSensorInfo->SensorReading = fan_rpm_raw;

        return 0;
    }

    pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE;
    pSensorInfo->EventFlags |= BIT5; // sensor was unavailable

    return -1;
}
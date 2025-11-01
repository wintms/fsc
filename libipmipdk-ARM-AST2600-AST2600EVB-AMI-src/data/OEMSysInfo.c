/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc. **
*** **
*** All Rights Reserved. **
*** **
*** **
*** File Name: OEMSysInfo.c **
*** Description: Source file for global system information access functions.
*** **
***************************************************************************
***************************************************************************
**************************************************************************/
#include <string.h>
#include "Debug.h"

#include "OEMSysInfo.h"
#include "OEMPLD.h"
#include "OEMFAN.h"

GlobalSystemInfo_T g_SystemInfo = {0};

/**
 * @fn OEM_UpdateFansAirflow
 * @brief Reads a fan's EEPROM to determine its airflow and caches it.
 * @return Void.
 */
void OEM_UpdateFansAirflow(void)
{
    int ret = -1;
    for (int i = 0; i < SYS_FAN_NUM_MAX; i++)
    {
        ret = OEM_GetFanTrayAirflow(i);
        if (ret < 0)
        {
            g_SystemInfo.globalFanInfo[i].airflow = AIRFLOW_UNKNOWN;
        }
        else
        {
            g_SystemInfo.globalFanInfo[i].airflow = ret;
        }
    }
}

/**
 * @fn OEM_UpdateSystemAirflow
 * @brief Determines the overall system airflow based on the individual fan airflows.
 * @details If 3 or more fans have the same airflow (F2B or B2F), that is set as the system airflow.
 *          Otherwise, the system airflow is set to unknown.
 */
void OEM_UpdateSystemAirflow(void)
{
    int f2b_count = 0;
    int b2f_count = 0;
    int i;

    // Iterate through all possible fan slots to count airflow directions.
    for (i = 0; i < SYS_FAN_NUM_MAX; i++)
    {
        // We only consider fans that have a known airflow.
        if (g_SystemInfo.globalFanInfo[i].airflow != AIRFLOW_UNKNOWN)
        {
            if (g_SystemInfo.globalFanInfo[i].airflow == AIRFLOW_F2B)
            {
                f2b_count++;
            }
            else if (g_SystemInfo.globalFanInfo[i].airflow == AIRFLOW_B2F)
            {
                b2f_count++;
            }
        }
    }

    // Determine system airflow based on the counts.
    if (f2b_count >= 3)
        g_SystemInfo.systemAirflow = AIRFLOW_F2B;
    else if (b2f_count >= 3)
        g_SystemInfo.systemAirflow = AIRFLOW_B2F;
    else
        g_SystemInfo.systemAirflow = AIRFLOW_UNKNOWN;
}

/**
 * @fn OEM_InitSysInfo
 * @brief Initializes the global system information structure.
 */
void OEM_InitSysInfo(void)
{
    memset(&g_SystemInfo, 0, sizeof(GlobalSystemInfo_T));

    // Update all fan trays airflow
    OEM_UpdateFansAirflow();

    // Update system airflow based on fans
    OEM_UpdateSystemAirflow();
}

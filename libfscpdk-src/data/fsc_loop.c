#include <stdio.h>
#include <string.h>

#include "IPMIConf.h"
#include "SensorAPI.h"
#include "OEMMisc.h"
#include "OEMFAN.h"
#include "OEMDBG.h"
#include "fsc.h"
#include "fsc_parser.h"
#include "fsc_utils.h"
#include "fsc_core.h"

/**
 * @fn FSCInitialize
 * @brief Initializes the Fan Speed Control (FSC) module.
 *
 * This function determines the correct fan control profile to use based on the
 * chassis type and airflow direction. It then parses the corresponding JSON
 * configuration file to load system information and sensor control profiles.
 * @param[out] verbose Pointer to store the debug verbosity level from the config.
 * @return 0 on success, -1 on failure.
 */
static int FSCInitialize(INT8U *verbose)
{
    char json_path[32] = {0};

    // Determine which profile to use
    int chassis_type = OEM_GetChassisType();
    int chassis_airflow = OEM_GetChassisAirFlow();

    if (CHASSIS_Z9964F == chassis_type)
    {
        if (AIRFLOW_F2B == chassis_airflow)
        {
            snprintf(json_path, sizeof(json_path), "%s", FSC_CONF_F2B_FILE);
        }
        else if (AIRFLOW_B2F == chassis_airflow)
        {
            snprintf(json_path, sizeof(json_path), "%s", FSC_CONF_B2F_FILE);
        }
    }
    else if (CHASSIS_Z9964FL == chassis_type)
    {
        snprintf(json_path, sizeof(json_path), "%s", FSC_CONF_LIQUID_FILE);
    }

    if ('\0' == json_path[0])
    {
        printf("FSC: Invalid chassis type %d and airflow %d.\n", chassis_type, chassis_airflow);
        return -1;
    }

    TINFO("Fan speed control configuration loaded: %s", json_path);

    if (0 != ParseDebugVerboseFromJson(json_path, verbose))
    {
        printf("FSC: Failed to parse 'debug_verbose' from %s.\n", json_path);
        return -1;
    }

    if (0 != ParseSystemInfoFromJson(json_path, &g_FscSystemInfo, *verbose))
    {
        printf("FSC: Failed to parse 'system_info' from %s.\n", json_path);
        return -1;
    }

    if (0 != ParseFSCProfileFromJson(json_path, &g_FscProfileInfo, *verbose))
    {
        printf("FSC: Failed to parse 'profile_info' from %s.\n", json_path);
        return -1;
    }

    return 0;
}

/**
 * @fn FSCUpdateOutputPWM
 * @brief Calculates the required fan PWM value based on all sensor readings.
 *
 * This function iterates through all configured temperature sensor profiles,
 * reads the current temperature for each, and calculates a required PWM value
 * using the defined algorithm (PID or Linear). It then determines the maximum
 * PWM value among all sensors, which becomes the final output PWM for the fans.
 * @param[out] pwm Pointer to store the final calculated PWM value.
 * @param[in] verbose Verbosity level for debug printing.
 * @param[in] BMCInst The BMC instance number.
 * @return 0 on success, -1 on failure.
 */
static int FSCUpdateOutputPWM(INT8U *pwm, INT8U verbose, int BMCInst)
{
    SensorInfo_T* pSensorInfo = NULL;
    INT8U pwm_value = 0;
    INT8U output_pwm = 0;
    int i = 0;

    for (i = 0; i < g_FscProfileInfo.TotalProfileNum; i++)
    {
        pFSCTempSensorInfo[i].SensorNumber = g_FscProfileInfo.ProfileInfo[i].SensorNum;
        pSensorInfo = API_GetSensorInfo(pFSCTempSensorInfo[i].SensorNumber, 0, BMCInst);

        if(pSensorInfo && pSensorInfo->Err != CC_DEST_UNAVAILABLE)
        {
            // Sensor Present
            pFSCTempSensorInfo[i].Present = pSensorInfo->IsSensorPresent;

            // CurrentTemp
            if ((pSensorInfo->EventFlags & 0x20) != 0x20) // Bit 5 -  Unable to read
            {
                pFSCTempSensorInfo[i].CurrentTemp = pSensorInfo->SensorReading;
            }
        }
        else
        {
            pFSCTempSensorInfo[i].Present = FALSE;
        }
        // MinPWM
        pFSCTempSensorInfo[i].MinPWM = g_FscSystemInfo.FanInitialPWM;
        // MaxPWM
        pFSCTempSensorInfo[i].MaxPWM = g_FscSystemInfo.FanMaxPWM;
        // Algorithm
        pFSCTempSensorInfo[i].Algorithm = g_FscProfileInfo.ProfileInfo[i].ProfileType;
        strcpy((char *)pFSCTempSensorInfo[i].Label,(char *)g_FscProfileInfo.ProfileInfo[i].Label);

        switch(pFSCTempSensorInfo[i].Algorithm)
        {
            case FSC_CTL_PID:
                pFSCTempSensorInfo[i].fscparam.pidparam.Pvalue = g_FscProfileInfo.ProfileInfo[i].PIDParameter.Kp;
                pFSCTempSensorInfo[i].fscparam.pidparam.Ivalue = g_FscProfileInfo.ProfileInfo[i].PIDParameter.Ki;
                pFSCTempSensorInfo[i].fscparam.pidparam.Dvalue = g_FscProfileInfo.ProfileInfo[i].PIDParameter.Kd;
                pFSCTempSensorInfo[i].fscparam.pidparam.SetPointType = g_FscProfileInfo.ProfileInfo[i].PIDParameter.SetPointType;
                pFSCTempSensorInfo[i].fscparam.pidparam.SetPoint = g_FscProfileInfo.ProfileInfo[i].PIDParameter.SetPoint; 
                break;

            case FSC_CTL_LINEAR:
                pFSCTempSensorInfo[i].fscparam.linearparam.TempMin = g_FscProfileInfo.ProfileInfo[i].LinearParameter.TempMin;
                pFSCTempSensorInfo[i].fscparam.linearparam.TempMax = g_FscProfileInfo.ProfileInfo[i].LinearParameter.TempMax;
                pFSCTempSensorInfo[i].fscparam.linearparam.PwmMin = g_FscProfileInfo.ProfileInfo[i].LinearParameter.PwmMin;
                pFSCTempSensorInfo[i].fscparam.linearparam.PwmMax = g_FscProfileInfo.ProfileInfo[i].LinearParameter.PwmMax;
                pFSCTempSensorInfo[i].fscparam.linearparam.FallingHyst = g_FscProfileInfo.ProfileInfo[i].LinearParameter.FallingHyst;
                break;

            case FSC_CTL_AMBIENT_BASE:
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.CurveType = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.CurveType;
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.LoadScenario = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.LoadScenario;
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.CoeffCount = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.CoeffCount;
                memcpy(pFSCTempSensorInfo[i].fscparam.ambientbaseparam.Coefficients, 
                       g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.Coefficients, 
                       sizeof(float) * MAX_POLYNOMIAL_COEFFS);
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.PointCount = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.PointCount;
                memcpy(pFSCTempSensorInfo[i].fscparam.ambientbaseparam.PiecewisePoints, 
                       g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.PiecewisePoints, 
                       sizeof(g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.PiecewisePoints));
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.FallingHyst = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.FallingHyst;
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.MaxRisingRate = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.MaxRisingRate;
                pFSCTempSensorInfo[i].fscparam.ambientbaseparam.MaxFallingRate = g_FscProfileInfo.ProfileInfo[i].AmbientBaseParameter.MaxFallingRate;
                break;

            default:
                FSCPRINT("Invalid Cooling algorithm. \n");
                return -1;
        }

        FSCGetPWMValue(&pwm_value, &pFSCTempSensorInfo[i], verbose, BMCInst);
        pFSCTempSensorInfo[i].CurrentPWM = pwm_value;

        if (pFSCTempSensorInfo[i].CurrentPWM > output_pwm)
        {
            output_pwm = pFSCTempSensorInfo[i].CurrentPWM;
        }
    }

    if(verbose > 0)
    {
        FSCPRINT("Output PWM = %d\n", output_pwm);
    }
    *pwm = output_pwm;

    return 0;
}

/**
 * @fn FanControlLoop
 * @brief The main loop for fan speed control.
 *
 * This function serves as the main entry point for the fan control logic.
 * It ensures one-time initialization, calculates the required PWM based on
 * current conditions, and then applies that PWM value to all chassis fans.
 * @param BMCInst The BMC instance number.
 * @return 0 on success, -1 on failure.
 */
int FanControlLoop(int BMCInst)
{
    static bool init_flag = false;
    INT8U pwm = 0;
    INT8U verbose = 0;
    int i = 0;

    if (!init_flag)
    {
        if (0 != FSCInitialize(&verbose))
        {
            printf("FSC: Initialization failed. Fan control will not run.\n");
            return -1;
        }
        TINFO("Fan speed control strategy started (%s)...", g_FscSystemInfo.FSCVersion);
        init_flag = true;
    }

    // overwrite verbose by oem command
    if(g_OEMDebugArray[OEM_DEBUG_Item_FSC])
    {
        verbose = 3;
    }

    FSCUpdateOutputPWM(&pwm, verbose, BMCInst);

    // Set the calculated PWM to all chassis fans
    for (i = 0; i < g_FscSystemInfo.ChassisFanUsedNum; i++)
    {
        OEM_SetChassisFanPWM(i, pwm);
    }

    return 0;
}
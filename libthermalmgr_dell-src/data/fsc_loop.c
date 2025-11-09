#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "IPMIConf.h"
#include "SensorAPI.h"
#include "OEMFAN.h"
#include "OEMDBG.h"
#include "fsc.h"
#include "fsc_parser.h"
#include "fsc_utils.h"
#include "fsc_core.h"
#include "fsc_common.h"
#include "fsc_math.h"

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
    int system_airflow = OEM_GetSystemAirflow();

    if (AIRFLOW_F2B == system_airflow)
    {
        snprintf(json_path, sizeof(json_path), "%s", FSC_CONF_F2B_FILE);
    }
    else
    {
        snprintf(json_path, sizeof(json_path), "%s", FSC_CONF_B2F_FILE);
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

    // Parse ambient calibration if present (safe defaults if missing)
    if (0 != ParseAmbientCalibrationFromJson(json_path, &g_AmbientCalibration, *verbose))
    {
        printf("FSC: Failed to parse 'ambient_calibration' from %s.\n", json_path);
        // Keep going with defaults set inside parser
    }

    if (0 != ParseFSCProfileFromJson(json_path, &g_FscProfileInfo, *verbose))
    {
        printf("FSC: Failed to parse 'profile_info' from %s.\n", json_path);
        return -1;
    }

    return 0;
}

// Forward declarations for separated functions
static int FSCReadAndAggregateSensors(INT8U verbose, int BMCInst);
static int FSCCalculateAllProfilePWMs(INT8U verbose, int BMCInst);
static INT8U FSCDetermineMaxPWM(INT8U verbose);

/**
 * @fn FSCReadSensorsForProfile
 * @brief Reads and aggregates sensors for a single profile
 */
static int FSCReadSensorsForProfile(int profile_idx, INT8U verbose, int BMCInst)
{
    if (0)
    {
        verbose = verbose;
    }

    SensorInfo_T* pSensorInfo = NULL;
    int sensor_count = g_FscProfileInfo.ProfileInfo[profile_idx].SensorCount;

    // Initialize defaults
    if (sensor_count <= 0)
    {
        sensor_count = 1;
        g_FscProfileInfo.ProfileInfo[profile_idx].SensorNums[0] =
            g_FscProfileInfo.ProfileInfo[profile_idx].SensorNum;
    }

    // For backward compatibility
    pFSCTempSensorInfo[profile_idx].SensorNumber =
        g_FscProfileInfo.ProfileInfo[profile_idx].SensorNums[0];

    long temp_sum = 0;
    int valid_count = 0;
    INT16S max_temp = -32768;
    int max_idx = -1;
    INT8U any_present = FALSE;

    // Read all sensors in the group
    for (int k = 0; k < sensor_count; k++)
    {
        INT8U s_num = g_FscProfileInfo.ProfileInfo[profile_idx].SensorNums[k];
        pSensorInfo = API_GetSensorInfo(s_num, 0, BMCInst);

        if (pSensorInfo && pSensorInfo->Err != CC_DEST_UNAVAILABLE)
        {
            if (pSensorInfo->IsSensorPresent)
            {
                any_present = TRUE;
                // Bit 5 - Unable to read
                if ((pSensorInfo->EventFlags & 0x20) != 0x20)
                {
                    INT16S t = pSensorInfo->SensorReading;
                    temp_sum += t;
                    valid_count++;

                    if (t > max_temp)
                    {
                        max_temp = t;
                        max_idx = k;
                    }
                }
            }
        }
    }

    // Store results
    pFSCTempSensorInfo[profile_idx].Present = any_present;
    if (valid_count > 0)
    {
        if (g_FscProfileInfo.ProfileInfo[profile_idx].AggregationMode == AGGREGATION_MAX &&
            max_idx >= 0)
        {
            pFSCTempSensorInfo[profile_idx].CurrentTemp = max_temp;
        }
        else
        {
            pFSCTempSensorInfo[profile_idx].CurrentTemp = (INT16S)(temp_sum / valid_count);
        }
    }
    else if (!any_present)
    {
        pFSCTempSensorInfo[profile_idx].Present = FALSE;
    }

    // Store metadata
    pFSCTempSensorInfo[profile_idx].MinPWM = g_FscSystemInfo.FanInitialPWM;
    pFSCTempSensorInfo[profile_idx].MaxPWM = g_FscSystemInfo.FanMaxPWM;
    pFSCTempSensorInfo[profile_idx].Algorithm = g_FscProfileInfo.ProfileInfo[profile_idx].ProfileType;
    strncpy((char *)pFSCTempSensorInfo[profile_idx].Label,
            (char *)g_FscProfileInfo.ProfileInfo[profile_idx].Label,
            sizeof(pFSCTempSensorInfo[profile_idx].Label) - 1);

    // Optional: Read power sensors for dynamic PID
    if (g_FscProfileInfo.ProfileInfo[profile_idx].ProfileType == FSC_CTL_PID &&
        g_FscProfileInfo.ProfileInfo[profile_idx].PIDAltCount > 0)
    {
        int pcount = g_FscProfileInfo.ProfileInfo[profile_idx].PowerSensorCount;
        int power_idx = 0;

        if (g_FscProfileInfo.ProfileInfo[profile_idx].AggregationMode == AGGREGATION_MAX &&
            max_idx >= 0 && max_idx < pcount)
        {
            power_idx = max_idx;
        }

        if (pcount > 0)
        {
            INT8U p_num = g_FscProfileInfo.ProfileInfo[profile_idx].PowerSensorNums[power_idx];
            pSensorInfo = API_GetSensorInfo(p_num, 0, BMCInst);

            if (pSensorInfo && pSensorInfo->Err != CC_DEST_UNAVAILABLE &&
                pSensorInfo->IsSensorPresent)
            {
                if ((pSensorInfo->EventFlags & 0x20) != 0x20)
                {
                    float selected_power = (float)pSensorInfo->SensorReading;

                    // Select bucket by power
                    for (int b = 0; b < g_FscProfileInfo.ProfileInfo[profile_idx].PIDAltCount; b++)
                    {
                        FSC_JSON_PID_POWER_BUCKET *pb = &g_FscProfileInfo.ProfileInfo[profile_idx].PIDAlt[b];
                        if (selected_power >= pb->PowerMin && selected_power <= pb->PowerMax)
                        {
                            // Copy selected bucket to runtime params
                            pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Pvalue = pb->Kp;
                            pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Ivalue = pb->Ki;
                            pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Dvalue = pb->Kd;

                            if (pb->SetPointType >= 0)
                            {
                                pFSCTempSensorInfo[profile_idx].fscparam.pidparam.SetPointType = pb->SetPointType;
                                pFSCTempSensorInfo[profile_idx].fscparam.pidparam.SetPoint = pb->SetPoint;
                            }
                            else if (pb->SetPoint != 0)
                            {
                                pFSCTempSensorInfo[profile_idx].fscparam.pidparam.SetPoint = pb->SetPoint;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    return FSC_OK;
}

/**
 * @fn FSCReadAndAggregateSensors
 * @brief Phase 1: Read all sensors and aggregate data
 */
static int FSCReadAndAggregateSensors(INT8U verbose, int BMCInst)
{
    int i;

    for (i = 0; i < g_FscProfileInfo.TotalProfileNum; i++)
    {
        if (FSCReadSensorsForProfile(i, verbose, BMCInst) != FSC_OK)
        {
            printf("FSC: Failed to read sensors for profile %d\n", i);
            return FSC_ERR_IO;
        }
    }

    if (verbose > 1)
    {
        FSCPRINT(" > Sensor reading phase complete\n");
    }

    return FSC_OK;
}

/**
 * @fn FSCCalculateProfilePWM
 * @brief Calculate PWM for a single profile
 */
static int FSCCalculateProfilePWM(int profile_idx, INT8U verbose, int BMCInst)
{
    INT8U pwm_value = 0;
    int ret;

    // Set algorithm parameters (only need for non-PID with power buckets)
    if (g_FscProfileInfo.ProfileInfo[profile_idx].ProfileType == FSC_CTL_PID &&
        g_FscProfileInfo.ProfileInfo[profile_idx].PIDAltCount == 0)
    {
        // Standard PID parameters
        pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Pvalue =
            g_FscProfileInfo.ProfileInfo[profile_idx].PIDParameter.Kp;
        pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Ivalue =
            g_FscProfileInfo.ProfileInfo[profile_idx].PIDParameter.Ki;
        pFSCTempSensorInfo[profile_idx].fscparam.pidparam.Dvalue =
            g_FscProfileInfo.ProfileInfo[profile_idx].PIDParameter.Kd;
        pFSCTempSensorInfo[profile_idx].fscparam.pidparam.SetPointType =
            g_FscProfileInfo.ProfileInfo[profile_idx].PIDParameter.SetPointType;
        pFSCTempSensorInfo[profile_idx].fscparam.pidparam.SetPoint =
            g_FscProfileInfo.ProfileInfo[profile_idx].PIDParameter.SetPoint;
    }
    else if (g_FscProfileInfo.ProfileInfo[profile_idx].ProfileType == FSC_CTL_POLYNOMIAL)
    {
        // Polynomial parameters
        memcpy(&pFSCTempSensorInfo[profile_idx].fscparam.ambientbaseparam,
               &g_FscProfileInfo.ProfileInfo[profile_idx].PolynomialParameter,
               sizeof(FSCPolynomial));
    }

    // Calculate PWM using the algorithm
    ret = FSCGetPWMValue(&pwm_value, &pFSCTempSensorInfo[profile_idx], verbose, BMCInst);
    if (ret == FSC_OK)
    {
        pFSCTempSensorInfo[profile_idx].CurrentPWM = pwm_value;
    }

    return ret;
}

/**
 * @fn FSCCalculateAllProfilePWMs
 * @brief Phase 2: Calculate PWM for all profiles
 */
static int FSCCalculateAllProfilePWMs(INT8U verbose, int BMCInst)
{
    int i;

    for (i = 0; i < g_FscProfileInfo.TotalProfileNum; i++)
    {
        if (FSCCalculateProfilePWM(i, verbose, BMCInst) != FSC_OK)
        {
            printf("FSC: Failed to calculate PWM for profile %d\n", i);
            return FSC_ERR_IO;
        }
    }

    if (verbose > 1)
    {
        FSCPRINT(" > PWM calculation phase complete\n");
    }

    return FSC_OK;
}

/**
 * @fn FSCDetermineMaxPWM
 * @brief Phase 3: Determine maximum PWM from all profiles
 */
static INT8U FSCDetermineMaxPWM(INT8U verbose)
{
    INT8U output_pwm = 0;
    int i;

    for (i = 0; i < g_FscProfileInfo.TotalProfileNum; i++)
    {
        if (pFSCTempSensorInfo[i].CurrentPWM > output_pwm)
        {
            output_pwm = pFSCTempSensorInfo[i].CurrentPWM;
        }
    }

    if (verbose > 0)
    {
        FSCPRINT(" > Determined max PWM: %d\n", output_pwm);
    }

    return output_pwm;
}

/**
 * @fn FSCUpdateOutputPWM
 * @brief Calculates the required fan PWM value based on all sensor readings.
 * Three-phase processing: 1) Read sensors, 2) Calculate PWMs, 3) Determine max
 */
static int FSCUpdateOutputPWM(INT8U *pwm, INT8U verbose, int BMCInst)
{
    // Phase 1: Read all sensors and aggregate data
    if (FSCReadAndAggregateSensors(verbose, BMCInst) != FSC_OK)
    {
        printf("FSC: Sensor read phase failed\n");
        return FSC_ERR_IO;
    }

    // Phase 2: Calculate PWM for all profiles
    if (FSCCalculateAllProfilePWMs(verbose, BMCInst) != FSC_OK)
    {
        printf("FSC: PWM calculation phase failed\n");
        return FSC_ERR_IO;
    }

    // Phase 3: Determine max PWM
    *pwm = FSCDetermineMaxPWM(verbose);

    return FSC_OK;
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
    static INT8U verbose = 0;
    INT8U pwm = 0;

    if (!init_flag)
    {
        init_flag = true;
        if (0 != FSCInitialize(&verbose))
        {
            TCRIT("FSC: Initialization failed. Fan control will not run.\n");
            return -1;
        }
        TINFO("Fan speed control strategy started (%s)...", g_FscSystemInfo.FSCVersion);
    }

    // overwrite verbose by oem command
    if(g_OEMDebugArray[OEM_DEBUG_Item_FSC])
    {
        verbose = 3;
    }

    FSCUpdateOutputPWM(&pwm, verbose, BMCInst);

    // Set the calculated PWM to all chassis fans
    OEM_SetAllFanTraysPWM(pwm);

    return 0;
}
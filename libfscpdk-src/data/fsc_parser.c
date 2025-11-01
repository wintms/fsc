#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "cJSON.h"
#include "OEMDBG.h"
#include "fsc_parser.h"
#include "fsc_utils.h"
#include "fsc_core.h"

FSC_JSON_SYSTEM_INFO            g_FscSystemInfo;
FSC_JSON_ALL_PROFILES_INFO      g_FscProfileInfo;

/**
 * @fn ReadFileToString
 * @brief Reads the entire content of a file into a dynamically allocated string.
 * @param[in] filename The path to the file to be read.
 * @return A pointer to a null-terminated string containing the file content on success,
 *         or NULL on failure. The caller is responsible for freeing the returned string.
 */
char* ReadFileToString(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if(file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if(fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }
    length = ftell(file);
    if(length < 0)
    {
        goto cleanup;
    }
    if(fseek(file, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char*)malloc((size_t)length + sizeof(""));
    if(content == NULL)
    {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if(read_chars != (size_t)length)
    {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if(file != NULL)
    {
        fclose(file);
    }

    return content;
}

/**
 * @fn ConvertcJSONToValue
 * @brief Extracts a value from a cJSON object based on a given key.
 * @param[in] pMap A pointer to the cJSON object to search within.
 * @param[in] pcKey The key string for the desired value.
 * @param[out] pValue A pointer to a variable to store the extracted value.
 *                    For numbers, this should be a pointer to a double.
 *                    For strings/objects, this should be a char buffer.
 * @return 0 on success, or a negative value on failure.
 *         -1: pMap, pcKey, pValue is NULL or key not found.
 */
int ConvertcJSONToValue(cJSON *  pMap, char *pcKey, void  *pValue)
{
    int       ret     = 0;
    cJSON*    pNode    = NULL;

    if(!pMap)
    {
        printf("pMap NULL\n");
        return  -1;
    }

    if(!pcKey)
    {
        printf("pcKey NULL\n");
        return  -1;
    }

    if(!pValue)
    {
        printf("pValue NULL\n");
        return  -1;
    }

    if((pNode = cJSON_GetObjectItem(pMap, pcKey)))
    {
        switch(pNode->type)
        {
            case cJSON_Number:
                *(double *)pValue = pNode->valuedouble;
                break;
            case cJSON_String:
                snprintf(pValue, LABEL_LENGTH_MAX, "%s", pNode->valuestring);
                break;

            default:
                ret = -1;
                break;
        }
    }

    return ret;
}

/**
 * @fn ParseDebugVerboseFromJson
 * @brief Parses the 'debug_verbose' level from a JSON configuration file.
 * @param[in] filename The path to the JSON configuration file.
 * @param[out] verbose Pointer to an INT8U to store the parsed verbosity level.
 * @return 0 on success, -1 on failure.
 */
int ParseDebugVerboseFromJson(char *filename, INT8U *verbose)
{
    char *file = NULL;
    cJSON *cjson_input = NULL;
    double Verbose;
    int ret = -1;

    file = ReadFileToString(filename);
    cjson_input = cJSON_Parse((const char *)file);

    if (cJSON_IsInvalid(cjson_input))
    {
        printf("fsc_parser: get invalid JSON string.\n");
        goto END;
    }

    if(ConvertcJSONToValue(cjson_input, "debug_verbose", &Verbose))
    {
        printf("fsc_parser: get debug_verbose error.\n");
        goto END;
    }
    *verbose = Verbose;
    ret = 0;

END:
    cJSON_Delete(cjson_input);
    if (file)
    {
        free(file);
    }
    return ret;
}

/**
 * @fn ParseSystemInfoFromJson
 * @brief Parses the 'system_info' object from a JSON configuration file.
 * @param[in] filename The path to the JSON configuration file.
 * @param[out] pFscSystemInfo Pointer to the FSC_JSON_SYSTEM_INFO structure to be populated.
 * @param[in] verbose Verbosity level for debug printing.
 * @return 0 on success, -1 on failure.
 */
int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose)
{
    char *file = NULL;
    cJSON *cjson_input = NULL;
    cJSON *pSystemInfo = NULL;
    char cString[LABEL_LENGTH_MAX]  = {0};
    double dTmp;
    int ret = -1;

    file = ReadFileToString(filename);
    cjson_input = cJSON_Parse(file);
    if (cJSON_IsInvalid(cjson_input))
    {
        printf("fsc_parser: get invalid JSON string.\n");
        goto END;
    }

    pSystemInfo = cJSON_GetObjectItem(cjson_input, "system_info");
    if(pSystemInfo == NULL)
    {
        printf("fsc_parser: get system_info error\n");
        goto END;
    }

    if(ConvertcJSONToValue(pSystemInfo, "chassis_fan_max_num", &dTmp))
    {
        printf("fsc_parser: get chassis_fan_max_num error\n");
        goto END;
    }
    pFscSystemInfo->ChassisFanMaxNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "chassis_fan_used_num", &dTmp))
    {
        printf("fsc_parser: get chassis_fan_used_num error\n");
        goto END;
    }
    pFscSystemInfo->ChassisFanUsedNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "chassis_fan_rotor_num", &dTmp))
    {
        printf("fsc_parser: get chassis_fan_rotor_num error\n");
        goto END;
    }
    pFscSystemInfo->ChassisFanRotorNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "chassis_fan_redundant_num", &dTmp))
    {
        printf("fsc_parser: get chassis_fan_redundant_num error\n");
        goto END;
    }
    pFscSystemInfo->ChassisFanRedundantNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "psu_fan_max_num", &dTmp))
    {
        printf("fsc_parser: get psu_fan_max_num error\n");
        goto END;
    }
    pFscSystemInfo->PSUFanMaxNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "psu_fan_used_num", &dTmp))
    {
        printf("fsc_parser: get psu_fan_used_num error\n");
        goto END;
    }
    pFscSystemInfo->PSUFanUsedNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "psu_fan_redundant_num", &dTmp))
    {
        printf("fsc_parser: get psu_fan_redundant_num error\n");
        goto END;
    }
    pFscSystemInfo->PSUFanRedundantNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "system_fan_airflow_max_num", &dTmp))
    {
        printf("fsc_parser: get system_fan_airflow_max_num error\n");
        goto END;
    }
    pFscSystemInfo->SystemMaxFanAirflowNum = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "system_fan_airflow", &dTmp))
    {
        printf("fsc_parser: get system_fan_airflow error\n");
        goto END;
    }
    pFscSystemInfo->SystemFanAirflow = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "fsc_mode", cString) || !strlen(cString))
    {
        printf("fsc_parser: get fsc_mode error\n");
        goto END;
    }

    if(strcmp(cString, CJSON_FSCMode_Auto) == 0)
    {
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_AUTO;
    }
    else if(strcmp(cString, CJSON_FSCMode_Manual) == 0)
    {
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_MANUAL;
    }

    if(ConvertcJSONToValue(pSystemInfo, "fsc_version", cString) || !strlen(cString))
    {
        printf("fsc_parser: get fsc_version error\n");
        goto END;
    }
    strncpy(pFscSystemInfo->FSCVersion, cString, sizeof(pFscSystemInfo->FSCVersion) - 1);
    pFscSystemInfo->FSCVersion[sizeof(pFscSystemInfo->FSCVersion) - 1] = '\0';

    if(ConvertcJSONToValue(pSystemInfo, "fan_initial_pwm", &dTmp))
    {
        printf("fsc_parser: get fan_initial_pwm error\n");
        goto END;
    }
    pFscSystemInfo->FanInitialPWM = (INT8U)dTmp;

    if(ConvertcJSONToValue(pSystemInfo, "fan_max_pwm", &dTmp))
    {
        printf("fsc_parser: get fan_max_pwm error\n");
        goto END;
    }
    pFscSystemInfo->FanMaxPWM = (INT8U)dTmp;

    ret = 0;

    if(g_OEMDebugArray[OEM_DEBUG_Item_FSC] > 1 || verbose > 1)
    {
        FSCPRINT(" > system_info: \n");
        FSCPRINT("  >> ChassisFanMaxNum          : %d\n", pFscSystemInfo->ChassisFanMaxNum);
        FSCPRINT("  >> ChassisFanUsedNum         : %d\n", pFscSystemInfo->ChassisFanUsedNum);
        FSCPRINT("  >> ChassisFanRotorNum        : %d\n", pFscSystemInfo->ChassisFanRotorNum);
        FSCPRINT("  >> PSUFanMaxNum              : %d\n", pFscSystemInfo->PSUFanMaxNum);
        FSCPRINT("  >> PSUFanUsedNum             : %d\n", pFscSystemInfo->PSUFanUsedNum);
        FSCPRINT("  >> SystemMaxFanAirflowNum    : %d\n", pFscSystemInfo->SystemMaxFanAirflowNum);
        FSCPRINT("  >> SystemFanAirflow          : %s\n", pFscSystemInfo->SystemFanAirflow ? "B2F" : "F2B");
        FSCPRINT("  >> FSCMode                   : %s\n", (pFscSystemInfo->FSCMode == FAN_CTL_MODE_AUTO) ? "Auto" : "Manual");
        FSCPRINT("  >> FanInitialPWM             : %d%%\n", pFscSystemInfo->FanInitialPWM);
    }

END:
    cJSON_Delete(cjson_input);
    if (file)
    {
        free(file);
    }
    return ret;
}

/**
 * @fn ParseFSCProfileFromJson
 * @brief Parses the 'profile_info' object from a JSON configuration file.
 * @param[in] filename The path to the JSON configuration file.
 * @param[out] pFscProfileInfo Pointer to the FSC_JSON_ALL_PROFILES_INFO structure to be populated.
 * @param[in] verbose Verbosity level for debug printing.
 * @return 0 on success, -1 on failure.
 */
int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose)
{
    char *file = NULL;
    cJSON *cjson_input = NULL;
    cJSON *pProfilesInfo = NULL;
    cJSON *pProfileListInfo = NULL;
    cJSON *pProfileItemInfo = NULL;

    int i;
    char cString[LABEL_LENGTH_MAX] = {0};
    double dTmp;
    int ret = -1;

    file = ReadFileToString(filename);
    cjson_input = cJSON_Parse(file);

    pProfilesInfo = cJSON_GetObjectItem(cjson_input, "profile_info");
    if(pProfilesInfo == NULL)
    {
        printf("fsc_parser: get system_info error\n");
        goto END;
    }

    pProfileListInfo = cJSON_GetObjectItem(pProfilesInfo, "profile_list");
    if(pProfileListInfo == NULL)
    {
        printf("fsc_parser: get temp_sensor_list error\n");
        goto END;
    }

    dTmp = cJSON_GetArraySize(pProfileListInfo);
    pFscProfileInfo->TotalProfileNum = (INT8U) dTmp;
    pFscProfileInfo->TotalLinearProfileNum = 0;
    pFscProfileInfo->TotalPIDProfileNum = 0;

    if (FSC_SENSOR_CNT_MAX < pFscProfileInfo->TotalProfileNum)
    {
        printf("fsc_parser: the number of sensors is out of range\n");
        goto END;
    }

    for(i = 0; i < pFscProfileInfo->TotalProfileNum; i++)
    {
        pProfileItemInfo = cJSON_GetArrayItem(pProfileListInfo, i);
        if(pProfileItemInfo == NULL)
        {
            printf("fsc_parser: get profile[%d] error\n", i);
            goto END;
        }

        if(ConvertcJSONToValue(pProfileItemInfo, "profile_index", &dTmp))
        {
            printf("fsc_parser: get profile_index error\n");
            goto END;
        }

        pFscProfileInfo->ProfileInfo[i].ProfileIndex = (INT8U) dTmp;

        if(ConvertcJSONToValue(pProfileItemInfo, "sensor_num", &dTmp))
        {
            printf("fsc_parser: get sensor_num error\n");
            goto END;
        }

        pFscProfileInfo->ProfileInfo[i].SensorNum = (INT8U) dTmp;

        if(ConvertcJSONToValue(pProfileItemInfo, "sensor_name", cString) || !strlen(cString))
        {
            printf("fsc_parser: get sensor_name error\n");
            goto END;
        }
        strncpy(pFscProfileInfo->ProfileInfo[i].SensorName, cString, sizeof(pFscProfileInfo->ProfileInfo[i].SensorName) - 1);
        pFscProfileInfo->ProfileInfo[i].SensorName[sizeof(pFscProfileInfo->ProfileInfo[i].SensorName) - 1] = '\0';

        if(ConvertcJSONToValue(pProfileItemInfo, "label", cString) || !strlen(cString))
        {
            printf("fsc_parser: get label error\n");
            goto END;
        }
        snprintf(pFscProfileInfo->ProfileInfo[i].Label, sizeof(pFscProfileInfo->ProfileInfo[i].Label), "%s", cString);

        if(ConvertcJSONToValue(pProfileItemInfo, "type", cString) || !strlen(cString))
        {
            printf("fsc_parser: get type error\n");
            goto END;
        }

        if(strcmp(cString, CJSON_ProfileType_Linear) == 0)
        {
            pFscProfileInfo->ProfileInfo[i].ProfileType = FSC_CTL_LINEAR;
            pFscProfileInfo->TotalLinearProfileNum++;

            if(ConvertcJSONToValue(pProfileItemInfo, "TempMin", &dTmp))
            {
                printf("fsc_parser: linear: get TempMin error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].LinearParameter.TempMin = dTmp;
            
            if(ConvertcJSONToValue(pProfileItemInfo, "TempMax", &dTmp))
            {
                printf("fsc_parser: linear: get TempMax error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].LinearParameter.TempMax = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "PwmMin", &dTmp))
            {
                printf("fsc_parser: linear: get PwmMin error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].LinearParameter.PwmMin = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "PwmMax", &dTmp))
            {
                printf("fsc_parser: linear: get PwmMax error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].LinearParameter.PwmMax = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "FallingHyst", &dTmp))
            {
                printf("fsc_parser: linear: get FallingHyst error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].LinearParameter.FallingHyst = dTmp;
        }
        else if(strcmp(cString, CJSON_ProfileType_PID) == 0)
        {
            pFscProfileInfo->ProfileInfo[i].ProfileType = FSC_CTL_PID;
            pFscProfileInfo->TotalPIDProfileNum++;

            if(ConvertcJSONToValue(pProfileItemInfo, "setpoint", &dTmp))
            {
                printf("fsc_parser: pid: get setpoint error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PIDParameter.SetPoint = dTmp;
            
            if(ConvertcJSONToValue(pProfileItemInfo, "setpoint_type", &dTmp))
            {
                printf("fsc_parser: pid: get setpoint_type error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PIDParameter.SetPointType = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "kp", &dTmp))
            {
                printf("fsc_parser: pid: get kp error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PIDParameter.Kp = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "ki", &dTmp))
            {
                printf("fsc_parser: pid: get ki error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PIDParameter.Ki = dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "kd", &dTmp))
            {
                printf("fsc_parser: pid: get kd error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PIDParameter.Kd = dTmp;
        }
    }

    ret = 0;

    if(g_OEMDebugArray[OEM_DEBUG_Item_FSC] > 1 || verbose > 1)
    {
        FSCPRINT(" > profile_info: \n");
        FSCPRINT("  >> TotalProfileNum           : %d\n", pFscProfileInfo->TotalProfileNum);
        FSCPRINT("  >> TotalPIDProfileNum        : %d\n", pFscProfileInfo->TotalPIDProfileNum);
        FSCPRINT("  >> TotalLinearProfileNum     : %d\n", pFscProfileInfo->TotalLinearProfileNum);

        for(i = 0; i < pFscProfileInfo->TotalProfileNum; i++)
        {
            FSCPRINT("  >> profile [%d]: \n", i);
            FSCPRINT("   >>> Label                   : %s\n", pFscProfileInfo->ProfileInfo[i].Label);
            FSCPRINT("   >>> ProfileIndex            : %d\n", pFscProfileInfo->ProfileInfo[i].ProfileIndex);
            FSCPRINT("   >>> SensorNum               : %d\n", pFscProfileInfo->ProfileInfo[i].SensorNum);
            FSCPRINT("   >>> SensorName              : %s\n", pFscProfileInfo->ProfileInfo[i].SensorName);
            FSCPRINT("   >>> ProfileType             : %d\n", pFscProfileInfo->ProfileInfo[i].ProfileType);

            if(pFscProfileInfo->ProfileInfo[i].ProfileType == FSC_CTL_PID)
            {
                FSCPRINT("   >>> ProfileParameter: \n");
                FSCPRINT("    >>>> SetPoint              : %f\n", pFscProfileInfo->ProfileInfo[i].PIDParameter.SetPoint);
                FSCPRINT("    >>>> SetPointType          : %d\n", pFscProfileInfo->ProfileInfo[i].PIDParameter.SetPointType);
                FSCPRINT("    >>>> Kp                    : %f\n", pFscProfileInfo->ProfileInfo[i].PIDParameter.Kp);
                FSCPRINT("    >>>> Ki                    : %f\n", pFscProfileInfo->ProfileInfo[i].PIDParameter.Ki);
                FSCPRINT("    >>>> Kd                    : %f\n", pFscProfileInfo->ProfileInfo[i].PIDParameter.Kd);
            }
            else if(pFscProfileInfo->ProfileInfo[i].ProfileType == FSC_CTL_LINEAR)
            {
                FSCPRINT("   >>> ProfileParameter: \n");
                FSCPRINT("    >>>> TempMin               : %d\n", pFscProfileInfo->ProfileInfo[i].LinearParameter.TempMin);
                FSCPRINT("    >>>> TempMax              : %d\n", pFscProfileInfo->ProfileInfo[i].LinearParameter.TempMax);
                FSCPRINT("    >>>> PwmMin                : %d\n", pFscProfileInfo->ProfileInfo[i].LinearParameter.PwmMin);
                FSCPRINT("    >>>> PwmMax                : %d\n", pFscProfileInfo->ProfileInfo[i].LinearParameter.PwmMax);
                FSCPRINT("    >>>> FallingHyst           : %d\n", pFscProfileInfo->ProfileInfo[i].LinearParameter.FallingHyst);
            }
            else
            {
                FSCPRINT("    >>>> Invalid pFscProfileInfo->ProfileInfo[%d].ProfileType: %d\n", i, pFscProfileInfo->ProfileInfo[i].ProfileType);
            }
        }
    }

END:
    cJSON_Delete(cjson_input);
    if (file)
    {
        free(file);
    }
    return ret;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "cJSON.h"
#include "OEMDBG.h"
#include "OEMFAN.h"
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

    file = fopen(filename, "rb");
    if(file == NULL)
    {
        return NULL;
    }

    if(fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }
    length = ftell(file);
    if(length < 0)
    {
        fclose(file);
        return NULL;
    }
    if(fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    content = (char*)malloc((size_t)length + 1);
    if(content == NULL)
    {
        fclose(file);
        return NULL;
    }

    read_chars = fread(content, sizeof(char), (size_t)length, file);
    fclose(file);
    if(read_chars != (size_t)length)
    {
        free(content);
        return NULL;
    }
    content[read_chars] = '\0';
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

    if(!pMap || !pcKey || !pValue)
    {
        printf("fsc_parser: invalid args in ConvertcJSONToValue\n");
        return  -1;
    }

    pNode = cJSON_GetObjectItem(pMap, pcKey);
    if(!pNode)
    {
        return -1;
    }

    if (cJSON_IsNumber(pNode))
    {
        *(double *)pValue = pNode->valuedouble;
    }
    else if (cJSON_IsString(pNode) && pNode->valuestring)
    {
        snprintf(pValue, LABEL_LENGTH_MAX, "%s", pNode->valuestring);
    }
    else
    {
        ret = -1;
    }

    return ret;
}


/**
 * @fn ParseAmbientCalibrationFromJson
 * @brief Parses ambient calibration parameters from JSON configuration file.
 * @param[in] filename The path to the JSON configuration file.
 * @param[out] pAmbientCalibration Pointer to store the parsed ambient calibration data.
 * @param[in] verbose Verbosity level for debug output.
 * @return 0 on success, or a negative value on failure.
 */
int ParseAmbientCalibrationFromJson(char *filename, FSCAmbientCalibration *pAmbientCalibration, INT8U verbose)
{
    char *file = NULL;
    cJSON *cjson_input = NULL;
    cJSON *pAmbientCalInfo = NULL;
    cJSON *pPointsArray = NULL;
    cJSON *pPointItem = NULL;
    
    int i;
    double dTmp;
    int ret = -1;

    file = ReadFileToString(filename);
    if (!file)
    {
        printf("fsc_parser: ambient calibration file read error\n");
        goto END;
    }
    cjson_input = cJSON_Parse(file);

    pAmbientCalInfo = cJSON_GetObjectItem(cjson_input, "ambient_calibration");
    if(pAmbientCalInfo == NULL)
    {
        printf("fsc_parser: ambient_calibration not found, using default values\n");
        // Set default values
        pAmbientCalibration->CalType = FSC_AMBIENT_CAL_POLYNOMIAL;
        pAmbientCalibration->CoeffCount = 2;
        pAmbientCalibration->Coefficients[0] = 0.0f;  // No offset
        pAmbientCalibration->Coefficients[1] = 1.0f;  // Direct mapping
        ret = 0;
        goto END;
    }

    if(ConvertcJSONToValue(pAmbientCalInfo, "cal_type", &dTmp))
    {
        printf("fsc_parser: get cal_type error\n");
        goto END;
    }
    pAmbientCalibration->CalType = (INT8U) dTmp;

    if(pAmbientCalibration->CalType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        if(ConvertcJSONToValue(pAmbientCalInfo, "coeff_count", &dTmp))
        {
            printf("fsc_parser: get coeff_count error\n");
            goto END;
        }
        pAmbientCalibration->CoeffCount = (INT8U) dTmp;

        // Parse coefficients array
        cJSON *pCoeffsArray = cJSON_GetObjectItem(pAmbientCalInfo, "coefficients");
        if(pCoeffsArray == NULL)
        {
            printf("fsc_parser: get coefficients array error\n");
            goto END;
        }

        for(i = 0; i < pAmbientCalibration->CoeffCount && i < MAX_POLYNOMIAL_COEFFS; i++)
        {
            cJSON *pCoeffItem = cJSON_GetArrayItem(pCoeffsArray, i);
            if(pCoeffItem == NULL)
            {
                printf("fsc_parser: get coefficient[%d] error\n", i);
                goto END;
            }
            pAmbientCalibration->Coefficients[i] = (float)cJSON_GetNumberValue(pCoeffItem);
        }
    }
    else if(pAmbientCalibration->CalType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        if(ConvertcJSONToValue(pAmbientCalInfo, "point_count", &dTmp))
        {
            printf("fsc_parser: get point_count error\n");
            goto END;
        }
        pAmbientCalibration->PointCount = (INT8U) dTmp;

        pPointsArray = cJSON_GetObjectItem(pAmbientCalInfo, "piecewise_points");
        if(pPointsArray == NULL)
        {
            printf("fsc_parser: get piecewise_points array error\n");
            goto END;
        }

        for(i = 0; i < pAmbientCalibration->PointCount && i < MAX_PIECEWISE_POINTS; i++)
        {
            pPointItem = cJSON_GetArrayItem(pPointsArray, i);
            if(pPointItem == NULL)
            {
                printf("fsc_parser: get piecewise_point[%d] error\n", i);
                goto END;
            }

            if(ConvertcJSONToValue(pPointItem, "pwm", &dTmp))
            {
                printf("fsc_parser: get pwm for point[%d] error\n", i);
                goto END;
            }
            pAmbientCalibration->PiecewisePoints[i].pwm = (float) dTmp;

            if(ConvertcJSONToValue(pPointItem, "delta_temp", &dTmp))
            {
                printf("fsc_parser: get delta_temp for point[%d] error\n", i);
                goto END;
            }
            pAmbientCalibration->PiecewisePoints[i].delta_temp = (float) dTmp;
        }
    }

    ret = 0;

    if(verbose > 1)
    {
        FSCPRINT(" > ambient_calibration: \n");
        FSCPRINT("  >> CalType                   : %d\n", pAmbientCalibration->CalType);
        
        if(pAmbientCalibration->CalType == FSC_AMBIENT_CAL_POLYNOMIAL)
        {
            FSCPRINT("  >> CoeffCount                : %d\n", pAmbientCalibration->CoeffCount);
            for(i = 0; i < pAmbientCalibration->CoeffCount; i++)
            {
                FSCPRINT("  >> Coefficient[%d]           : %f\n", i, pAmbientCalibration->Coefficients[i]);
            }
        }
        else if(pAmbientCalibration->CalType == FSC_AMBIENT_CAL_PIECEWISE)
        {
            FSCPRINT("  >> PointCount                : %d\n", pAmbientCalibration->PointCount);
            for(i = 0; i < pAmbientCalibration->PointCount; i++)
            {
                FSCPRINT("  >> Point[%d]: PWM=%d, DeltaT=%f\n", i,
                        pAmbientCalibration->PiecewisePoints[i].pwm,
                        pAmbientCalibration->PiecewisePoints[i].delta_temp);
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
    if (!file)
    {
        printf("fsc_parser: debug_verbose read error.\n");
        goto END;
    }
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
    if (!file)
    {
        printf("fsc_parser: system_info file read error\n");
        goto END;
    }
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

    if(ConvertcJSONToValue(pSystemInfo, "fsc_mode", cString) || !strlen(cString))
    {
        // fsc_mode optional, default to auto
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_AUTO;
    }
    else if(strcmp(cString, CJSON_FSCMode_Auto) == 0)
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

    if(verbose > 1)
    {
        FSCPRINT(" > system_info: \n");
        FSCPRINT("  >> FSCMode                   : %s\n", (pFscSystemInfo->FSCMode == FAN_CTL_MODE_AUTO) ? "Auto" : "Manual");
        FSCPRINT("  >> FanInitialPWM             : %d%%\n", pFscSystemInfo->FanInitialPWM);
        FSCPRINT("  >> FanMaxPWM                 : %d%%\n", pFscSystemInfo->FanMaxPWM);
        FSCPRINT("  >> FSCVersion                : %s\n", pFscSystemInfo->FSCVersion);
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
    if (!file)
    {
        printf("fsc_parser: profile_info file read error\n");
        goto END;
    }
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

        // Initialize multi-sensor grouping defaults
        pFscProfileInfo->ProfileInfo[i].SensorCount = 0;
        memset(pFscProfileInfo->ProfileInfo[i].SensorNums, 0, sizeof(pFscProfileInfo->ProfileInfo[i].SensorNums));

        // Prefer sensor_nums array if provided; otherwise fall back to single sensor_num
        cJSON *pSensorNumsArray = cJSON_GetObjectItem(pProfileItemInfo, "sensor_nums");
        if (pSensorNumsArray && cJSON_IsArray(pSensorNumsArray))
        {
            int arrSize = cJSON_GetArraySize(pSensorNumsArray);
            if (arrSize <= 0)
            {
                printf("fsc_parser: sensor_nums array is empty\n");
                goto END;
            }
            if (arrSize > MAX_SENSOR_GROUP_SIZE)
            {
                arrSize = MAX_SENSOR_GROUP_SIZE;
            }
            pFscProfileInfo->ProfileInfo[i].SensorCount = (INT8U)arrSize;
            for (int j = 0; j < arrSize; j++)
            {
                cJSON *pItem = cJSON_GetArrayItem(pSensorNumsArray, j);
                if (!pItem)
                {
                    printf("fsc_parser: get sensor_nums[%d] error\n", j);
                    goto END;
                }
                pFscProfileInfo->ProfileInfo[i].SensorNums[j] = (INT8U)cJSON_GetNumberValue(pItem);
            }
            // For backward compatibility, set SensorNum to the first element
            pFscProfileInfo->ProfileInfo[i].SensorNum = pFscProfileInfo->ProfileInfo[i].SensorNums[0];
        }
        else
        {
            if(ConvertcJSONToValue(pProfileItemInfo, "sensor_num", &dTmp))
            {
                printf("fsc_parser: get sensor_num error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].SensorNum = (INT8U) dTmp;
            pFscProfileInfo->ProfileInfo[i].SensorCount = 1;
            pFscProfileInfo->ProfileInfo[i].SensorNums[0] = pFscProfileInfo->ProfileInfo[i].SensorNum;
        }

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

        if(strcmp(cString, CJSON_ProfileType_PID) == 0)
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
        else if(strcmp(cString, CJSON_ProfileType_Polynomial) == 0)
        {
            pFscProfileInfo->ProfileInfo[i].ProfileType = FSC_CTL_POLYNOMIAL;

            if(ConvertcJSONToValue(pProfileItemInfo, "curve_type", &dTmp))
            {
                printf("fsc_parser: ambient_base: get curve_type error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType = (INT8U) dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "load_scenario", &dTmp))
            {
                printf("fsc_parser: ambient_base: get load_scenario error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PolynomialParameter.LoadScenario = (INT8U) dTmp;

            if(pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
            {
                if(ConvertcJSONToValue(pProfileItemInfo, "coeff_count", &dTmp))
                {
                    printf("fsc_parser: ambient_base: get coeff_count error\n");
                    goto END;
                }
                pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CoeffCount = (INT8U) dTmp;

                // Parse coefficients array
                cJSON *pCoeffsArray = cJSON_GetObjectItem(pProfileItemInfo, "coefficients");
                if(pCoeffsArray == NULL)
                {
                    printf("fsc_parser: ambient_base: get coefficients array error\n");
                    goto END;
                }

                int j;
                for(j = 0; j < pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CoeffCount && j < MAX_POLYNOMIAL_COEFFS; j++)
                {
                    cJSON *pCoeffItem = cJSON_GetArrayItem(pCoeffsArray, j);
                    if(pCoeffItem == NULL)
                    {
                        printf("fsc_parser: ambient_base: get coefficient[%d] error\n", j);
                        goto END;
                    }
                    pFscProfileInfo->ProfileInfo[i].PolynomialParameter.Coefficients[j] = (float)cJSON_GetNumberValue(pCoeffItem);
                }
            }
            else if(pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType == FSC_AMBIENT_CAL_PIECEWISE)
            {
                if(ConvertcJSONToValue(pProfileItemInfo, "point_count", &dTmp))
                {
                    printf("fsc_parser: ambient_base: get point_count error\n");
                    goto END;
                }
                pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PointCount = (INT8U) dTmp;

                cJSON *pPointsArray = cJSON_GetObjectItem(pProfileItemInfo, "piecewise_points");
                if(pPointsArray == NULL)
                {
                    printf("fsc_parser: ambient_base: get piecewise_points array error\n");
                    goto END;
                }

                int j;
                for(j = 0; j < pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PointCount && j < MAX_PIECEWISE_POINTS; j++)
                {
                    cJSON *pPointItem = cJSON_GetArrayItem(pPointsArray, j);
                    if(pPointItem == NULL)
                    {
                        printf("fsc_parser: ambient_base: get piecewise_point[%d] error\n", j);
                        goto END;
                    }

                    if(ConvertcJSONToValue(pPointItem, "temp", &dTmp))
                    {
                        printf("fsc_parser: ambient_base: get temp for point[%d] error\n", j);
                        goto END;
                    }
                    pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PiecewisePoints[j].temp = (float) dTmp;

                    if(ConvertcJSONToValue(pPointItem, "pwm", &dTmp))
                    {
                        printf("fsc_parser: ambient_base: get pwm for point[%d] error\n", j);
                        goto END;
                    }
                    pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PiecewisePoints[j].pwm = (float) dTmp;
                }
            }

            if(ConvertcJSONToValue(pProfileItemInfo, "falling_hyst", &dTmp))
            {
                printf("fsc_parser: ambient_base: get falling_hyst error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PolynomialParameter.FallingHyst = (float) dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "max_rising_rate", &dTmp))
            {
                printf("fsc_parser: ambient_base: get max_rising_rate error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PolynomialParameter.MaxRisingRate = (INT8U) dTmp;

            if(ConvertcJSONToValue(pProfileItemInfo, "max_falling_rate", &dTmp))
            {
                printf("fsc_parser: ambient_base: get max_falling_rate error\n");
                goto END;
            }
            pFscProfileInfo->ProfileInfo[i].PolynomialParameter.MaxFallingRate = (INT8U) dTmp;
        }
    }

    ret = 0;

    if(verbose > 1)
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
            else if(pFscProfileInfo->ProfileInfo[i].ProfileType == FSC_CTL_POLYNOMIAL)
            {
                FSCPRINT("   >>> ProfileParameter: \n");
                FSCPRINT("    >>>> CurveType             : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType);
                FSCPRINT("    >>>> LoadScenario          : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.LoadScenario);
                FSCPRINT("    >>>> FallingHyst           : %f\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.FallingHyst);
                FSCPRINT("    >>>> MaxRisingRate         : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.MaxRisingRate);
                FSCPRINT("    >>>> MaxFallingRate        : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.MaxFallingRate);
                
                if(pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
                {
                    FSCPRINT("    >>>> CoeffCount            : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CoeffCount);
                    int j;
                    for(j = 0; j < pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CoeffCount; j++)
                    {
                        FSCPRINT("    >>>> Coefficient[%d]       : %f\n", j, pFscProfileInfo->ProfileInfo[i].PolynomialParameter.Coefficients[j]);
                    }
                }
                else if(pFscProfileInfo->ProfileInfo[i].PolynomialParameter.CurveType == FSC_AMBIENT_CAL_PIECEWISE)
                {
                    FSCPRINT("    >>>> PointCount            : %d\n", pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PointCount);
                    int j;
                    for(j = 0; j < pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PointCount; j++)
                    {
                        FSCPRINT("    >>>> Point[%d]: Temp=%d, PWM=%d\n", j, 
                                pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PiecewisePoints[j].temp,
                                pFscProfileInfo->ProfileInfo[i].PolynomialParameter.PiecewisePoints[j].pwm);
                    }
                }
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
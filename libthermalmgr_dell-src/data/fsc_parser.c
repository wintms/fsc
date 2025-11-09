/*************************************************************************
 *
 * fsc_parser.c
 * Configuration file parser using JSON format
 * Refactored with resource management and simplified error handling
 *
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "cJSON.h"
#include "OEMDBG.h"
#include "OEMFAN.h"
#include "fsc_parser.h"
#include "fsc_utils.h"
#include "fsc_common.h"
#include "fsc_math.h"

FSC_JSON_SYSTEM_INFO            g_FscSystemInfo;
FSC_JSON_ALL_PROFILES_INFO      g_FscProfileInfo;

// Internal helper macros
#define DEBUG_PARSE(verbose, fmt, ...) \
    do { \
        if (verbose > 1) { \
            FSCPRINT(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define PARSE_DOUBLE_OR_GOTO(json, key, var, msg) \
    do { \
        double _dtmp; \
        if (ConvertcJSONToValue(json, key, &_dtmp) != 0) { \
            printf("fsc_parser: %s\n", msg); \
            goto cleanup; \
        } \
        var = _dtmp; \
    } while(0)

#define PARSE_STRING_OR_GOTO(json, key, var, size, msg) \
    do { \
        char _stmp[LABEL_LENGTH_MAX] = {0}; \
        if (ConvertcJSONToValue(json, key, _stmp) != 0 || !strlen(_stmp)) { \
            printf("fsc_parser: %s\n", msg); \
            goto cleanup; \
        } \
        strncpy(var, _stmp, (size) - 1); \
        var[(size) - 1] = '\0'; \
    } while(0)

/**
 * @fn ParseAmbientCalibrationFromJson
 * @brief Parses ambient calibration parameters from JSON configuration file.
 */
int ParseAmbientCalibrationFromJson(char *filename, FSCAmbientCalibration *pAmbientCalibration, INT8U verbose)
{
    FscParseContext ctx = {0};
    cJSON *pAmbientCalInfo = NULL;
    cJSON *pPointsArray = NULL;
    int ret = FSC_ERR_PARSE;
    int i;

    if (FscParseContext_Init(&ctx, filename) != FSC_OK)
    {
        printf("fsc_parser: Failed to load %s\n", filename);
        return FSC_ERR_IO;
    }

    pAmbientCalInfo = cJSON_GetObjectItem(ctx.json_root, "ambient_calibration");
    if (pAmbientCalInfo == NULL)
    {
        printf("fsc_parser: ambient_calibration not found, using default values\n");
        pAmbientCalibration->CalType = FSC_AMBIENT_CAL_POLYNOMIAL;
        pAmbientCalibration->CoeffCount = 2;
        pAmbientCalibration->Coefficients[0] = 0.0f;
        pAmbientCalibration->Coefficients[1] = 1.0f;
        ret = FSC_OK;
        goto cleanup;
    }

    double dTmp;
    PARSE_DOUBLE_OR_GOTO(pAmbientCalInfo, "cal_type", dTmp, "get cal_type error");
    pAmbientCalibration->CalType = (INT8U)dTmp;

    if (pAmbientCalibration->CalType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        PARSE_DOUBLE_OR_GOTO(pAmbientCalInfo, "coeff_count", dTmp, "get coeff_count error");
        pAmbientCalibration->CoeffCount = (INT8U)dTmp;

        cJSON *pCoeffsArray = cJSON_GetObjectItem(pAmbientCalInfo, "coefficients");
        if (pCoeffsArray == NULL)
        {
            printf("fsc_parser: get coefficients array error\n");
            goto cleanup;
        }

        for (i = 0; i < pAmbientCalibration->CoeffCount && i < MAX_POLYNOMIAL_COEFFS; i++)
        {
            cJSON *pCoeffItem = cJSON_GetArrayItem(pCoeffsArray, i);
            if (pCoeffItem == NULL)
            {
                printf("fsc_parser: get coefficient[%d] error\n", i);
                goto cleanup;
            }
            pAmbientCalibration->Coefficients[i] = (float)cJSON_GetNumberValue(pCoeffItem);
        }
    }
    else if (pAmbientCalibration->CalType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        PARSE_DOUBLE_OR_GOTO(pAmbientCalInfo, "point_count", dTmp, "get point_count error");
        pAmbientCalibration->PointCount = (INT8U)dTmp;

        pPointsArray = cJSON_GetObjectItem(pAmbientCalInfo, "piecewise_points");
        if (pPointsArray == NULL)
        {
            printf("fsc_parser: get piecewise_points array error\n");
            goto cleanup;
        }

        for (i = 0; i < pAmbientCalibration->PointCount && i < MAX_PIECEWISE_POINTS; i++)
        {
            cJSON *pPointItem = cJSON_GetArrayItem(pPointsArray, i);
            if (pPointItem == NULL)
            {
                printf("fsc_parser: get piecewise_point[%d] error\n", i);
                goto cleanup;
            }

            PARSE_DOUBLE_OR_GOTO(pPointItem, "pwm", dTmp, "get pwm");
            pAmbientCalibration->PiecewisePoints[i].pwm = (float)dTmp;

            PARSE_DOUBLE_OR_GOTO(pPointItem, "delta_temp", dTmp, "get delta_temp");
            pAmbientCalibration->PiecewisePoints[i].delta_temp = (float)dTmp;
        }
    }

    ret = FSC_OK;

    DEBUG_PARSE(verbose, " > Parsed ambient_calibration: CalType=%d\n", pAmbientCalibration->CalType);

cleanup:
    FscParseContext_Cleanup(&ctx);
    return ret;
}

/**
 * @fn ParseDebugVerboseFromJson
 * @brief Parses the 'debug_verbose' level from a JSON configuration file.
 */
int ParseDebugVerboseFromJson(char *filename, INT8U *verbose)
{
    FscParseContext ctx = {0};
    double Verbose;
    int ret = FSC_ERR_PARSE;

    if (FscParseContext_Init(&ctx, filename) != FSC_OK)
    {
        printf("fsc_parser: Failed to load %s\n", filename);
        return FSC_ERR_IO;
    }

    if (ConvertcJSONToValue(ctx.json_root, "debug_verbose", &Verbose) != 0)
    {
        printf("fsc_parser: get debug_verbose error\n");
        goto cleanup;
    }

    *verbose = (INT8U)Verbose;
    ret = FSC_OK;

cleanup:
    FscParseContext_Cleanup(&ctx);
    return ret;
}

/**
 * @fn ParseArrayToUint8
 * @brief Helper to parse JSON array to uint8_t array
 */
static int ParseArrayToUint8(cJSON *pArray, INT8U *dest, int max_count, int *out_count)
{
    int arrSize;
    int i;

    if (!pArray || !cJSON_IsArray(pArray))
    {
        return -1;
    }

    arrSize = cJSON_GetArraySize(pArray);
    if (arrSize <= 0)
    {
        return -1;
    }

    if (arrSize > max_count)
    {
        arrSize = max_count;
    }

    for (i = 0; i < arrSize; i++)
    {
        cJSON *pItem = cJSON_GetArrayItem(pArray, i);
        if (!pItem)
        {
            return -1;
        }
        dest[i] = (INT8U)cJSON_GetNumberValue(pItem);
    }

    *out_count = arrSize;
    return 0;
}

/**
 * @fn ParsePIDProfile
 * @brief Helper to parse PID profile parameters
 */
static int ParsePIDProfile(FSC_JSON_PROFILE_INFO *pProfile, cJSON *pProfileItem)
{
    double dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "setpoint", dTmp, "pid: get setpoint");
    pProfile->PIDParameter.SetPoint = dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "setpoint_type", dTmp, "pid: get setpoint_type");
    pProfile->PIDParameter.SetPointType = (signed char)dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "kp", dTmp, "pid: get kp");
    pProfile->PIDParameter.Kp = dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "ki", dTmp, "pid: get ki");
    pProfile->PIDParameter.Ki = dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "kd", dTmp, "pid: get kd");
    pProfile->PIDParameter.Kd = dTmp;

    // Optional: pid_power_buckets
    cJSON *pPidBuckets = cJSON_GetObjectItem(pProfileItem, "pid_power_buckets");
    if (pPidBuckets && cJSON_IsArray(pPidBuckets))
    {
        int bSize = cJSON_GetArraySize(pPidBuckets);
        if (bSize > MAX_PID_POWER_BUCKETS) bSize = MAX_PID_POWER_BUCKETS;
        pProfile->PIDAltCount = (INT8U)bSize;

        for (int j = 0; j < bSize; j++)
        {
            cJSON *pB = cJSON_GetArrayItem(pPidBuckets, j);
            if (!pB) goto cleanup;

            double tval;
            PARSE_DOUBLE_OR_GOTO(pB, "power_min", tval, "pid_bucket power_min");
            pProfile->PIDAlt[j].PowerMin = (float)tval;

            PARSE_DOUBLE_OR_GOTO(pB, "power_max", tval, "pid_bucket power_max");
            pProfile->PIDAlt[j].PowerMax = (float)tval;

            PARSE_DOUBLE_OR_GOTO(pB, "kp", tval, "pid_bucket kp");
            pProfile->PIDAlt[j].Kp = tval;

            PARSE_DOUBLE_OR_GOTO(pB, "ki", tval, "pid_bucket ki");
            pProfile->PIDAlt[j].Ki = tval;

            PARSE_DOUBLE_OR_GOTO(pB, "kd", tval, "pid_bucket kd");
            pProfile->PIDAlt[j].Kd = tval;

            // Optional overrides
            if (ConvertcJSONToValue(pB, "setpoint", &tval) == 0)
                pProfile->PIDAlt[j].SetPoint = tval;
            else
                pProfile->PIDAlt[j].SetPoint = 0;

            if (ConvertcJSONToValue(pB, "setpoint_type", &tval) == 0)
                pProfile->PIDAlt[j].SetPointType = (signed char)tval;
            else
                pProfile->PIDAlt[j].SetPointType = -1;
        }
    }

    return 0;

cleanup:
    return -1;
}

/**
 * @fn ParsePolynomialProfile
 * @brief Helper to parse polynomial profile parameters
 */
static int ParsePolynomialProfile(FSC_JSON_PROFILE_INFO *pProfile, cJSON *pProfileItem)
{
    double dTmp;
    int j;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "curve_type", dTmp, "ambient_base: get curve_type");
    pProfile->PolynomialParameter.CurveType = (INT8U)dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "load_scenario", dTmp, "ambient_base: get load_scenario");
    pProfile->PolynomialParameter.LoadScenario = (INT8U)dTmp;

    if (pProfile->PolynomialParameter.CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        PARSE_DOUBLE_OR_GOTO(pProfileItem, "coeff_count", dTmp, "ambient_base: get coeff_count");
        pProfile->PolynomialParameter.CoeffCount = (INT8U)dTmp;

        cJSON *pCoeffsArray = cJSON_GetObjectItem(pProfileItem, "coefficients");
        if (pCoeffsArray == NULL) goto cleanup;

        for (j = 0; j < pProfile->PolynomialParameter.CoeffCount && j < MAX_POLYNOMIAL_COEFFS; j++)
        {
            cJSON *pCoeffItem = cJSON_GetArrayItem(pCoeffsArray, j);
            if (pCoeffItem == NULL) goto cleanup;
            pProfile->PolynomialParameter.Coefficients[j] = (float)cJSON_GetNumberValue(pCoeffItem);
        }
    }
    else if (pProfile->PolynomialParameter.CurveType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        PARSE_DOUBLE_OR_GOTO(pProfileItem, "point_count", dTmp, "ambient_base: get point_count");
        pProfile->PolynomialParameter.PointCount = (INT8U)dTmp;

        cJSON *pPointsArray = cJSON_GetObjectItem(pProfileItem, "piecewise_points");
        if (pPointsArray == NULL) goto cleanup;

        for (j = 0; j < pProfile->PolynomialParameter.PointCount && j < MAX_PIECEWISE_POINTS; j++)
        {
            cJSON *pPointItem = cJSON_GetArrayItem(pPointsArray, j);
            if (pPointItem == NULL) goto cleanup;

            PARSE_DOUBLE_OR_GOTO(pPointItem, "temp", dTmp, "ambient_base: get temp");
            pProfile->PolynomialParameter.PiecewisePoints[j].temp = (float)dTmp;

            PARSE_DOUBLE_OR_GOTO(pPointItem, "pwm", dTmp, "ambient_base: get pwm");
            pProfile->PolynomialParameter.PiecewisePoints[j].pwm = (float)dTmp;
        }
    }

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "falling_hyst", dTmp, "ambient_base: get falling_hyst");
    pProfile->PolynomialParameter.FallingHyst = (float)dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "max_rising_rate", dTmp, "ambient_base: get max_rising_rate");
    pProfile->PolynomialParameter.MaxRisingRate = (INT8U)dTmp;

    PARSE_DOUBLE_OR_GOTO(pProfileItem, "max_falling_rate", dTmp, "ambient_base: get max_falling_rate");
    pProfile->PolynomialParameter.MaxFallingRate = (INT8U)dTmp;

    return 0;

cleanup:
    return -1;
}

/**
 * @fn ParseFSCProfileFromJson
 * @brief Parses the 'profile_info' object from a JSON configuration file.
 */
int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose)
{
    FscParseContext ctx = {0};
    cJSON *pProfilesInfo = NULL;
    cJSON *pProfileListInfo = NULL;
    int ret = FSC_ERR_PARSE;
    int i;
    char cString[LABEL_LENGTH_MAX];

    if (FscParseContext_Init(&ctx, filename) != FSC_OK)
    {
        printf("fsc_parser: Failed to load %s\n", filename);
        return FSC_ERR_IO;
    }

    pProfilesInfo = cJSON_GetObjectItem(ctx.json_root, "profile_info");
    if (pProfilesInfo == NULL) goto cleanup;

    pProfileListInfo = cJSON_GetObjectItem(pProfilesInfo, "profile_list");
    if (pProfileListInfo == NULL) goto cleanup;

    double dTmp = cJSON_GetArraySize(pProfileListInfo);
    pFscProfileInfo->TotalProfileNum = (INT8U)dTmp;
    pFscProfileInfo->TotalLinearProfileNum = 0;
    pFscProfileInfo->TotalPIDProfileNum = 0;

    if (FSC_SENSOR_CNT_MAX < pFscProfileInfo->TotalProfileNum) goto cleanup;

    for (i = 0; i < pFscProfileInfo->TotalProfileNum; i++)
    {
        cJSON *pProfileItem = cJSON_GetArrayItem(pProfileListInfo, i);
        if (pProfileItem == NULL) goto cleanup;

        PARSE_DOUBLE_OR_GOTO(pProfileItem, "profile_index", dTmp, "get profile_index");
        pFscProfileInfo->ProfileInfo[i].ProfileIndex = (INT8U)dTmp;

        // Initialize defaults
        pFscProfileInfo->ProfileInfo[i].SensorCount = 0;
        memset(pFscProfileInfo->ProfileInfo[i].SensorNums, 0, sizeof(pFscProfileInfo->ProfileInfo[i].SensorNums));
        pFscProfileInfo->ProfileInfo[i].AggregationMode = AGGREGATION_AVERAGE;
        pFscProfileInfo->ProfileInfo[i].PowerSensorCount = 0;
        memset(pFscProfileInfo->ProfileInfo[i].PowerSensorNums, 0, sizeof(pFscProfileInfo->ProfileInfo[i].PowerSensorNums));
        pFscProfileInfo->ProfileInfo[i].PIDAltCount = 0;

        // Sensor numbers (array preferred, fallback to single sensor_num)
        cJSON *pSensorNumsArray = cJSON_GetObjectItem(pProfileItem, "sensor_nums");
        if (pSensorNumsArray && cJSON_IsArray(pSensorNumsArray))
        {
            int result = ParseArrayToUint8(pSensorNumsArray,
                                          pFscProfileInfo->ProfileInfo[i].SensorNums,
                                          MAX_SENSOR_GROUP_SIZE,
                                          (int *)&pFscProfileInfo->ProfileInfo[i].SensorCount);
            if (result != 0) goto cleanup;
            pFscProfileInfo->ProfileInfo[i].SensorNum = pFscProfileInfo->ProfileInfo[i].SensorNums[0];
        }
        else
        {
            PARSE_DOUBLE_OR_GOTO(pProfileItem, "sensor_num", dTmp, "get sensor_num");
            pFscProfileInfo->ProfileInfo[i].SensorNum = (INT8U)dTmp;
            pFscProfileInfo->ProfileInfo[i].SensorCount = 1;
            pFscProfileInfo->ProfileInfo[i].SensorNums[0] = pFscProfileInfo->ProfileInfo[i].SensorNum;
        }

        // Aggregation mode
        if (ConvertcJSONToValue(pProfileItem, "aggregation", cString) == 0 && strlen(cString))
        {
            if (strcmp(cString, "max") == 0)
                pFscProfileInfo->ProfileInfo[i].AggregationMode = AGGREGATION_MAX;
            else
                pFscProfileInfo->ProfileInfo[i].AggregationMode = AGGREGATION_AVERAGE;
        }

        // Optional: power_sensor_nums
        cJSON *pPowerSensorNumsArray = cJSON_GetObjectItem(pProfileItem, "power_sensor_nums");
        if (pPowerSensorNumsArray && cJSON_IsArray(pPowerSensorNumsArray))
        {
            ParseArrayToUint8(pPowerSensorNumsArray,
                             pFscProfileInfo->ProfileInfo[i].PowerSensorNums,
                             MAX_SENSOR_GROUP_SIZE,
                             (int *)&pFscProfileInfo->ProfileInfo[i].PowerSensorCount);
        }

        PARSE_STRING_OR_GOTO(pProfileItem, "label", pFscProfileInfo->ProfileInfo[i].Label,
                            sizeof(pFscProfileInfo->ProfileInfo[i].Label), "get label");

        PARSE_STRING_OR_GOTO(pProfileItem, "type", cString, sizeof(cString), "get type");

        if (strcmp(cString, CJSON_ProfileType_PID) == 0)
        {
            pFscProfileInfo->ProfileInfo[i].ProfileType = FSC_CTL_PID;
            pFscProfileInfo->TotalPIDProfileNum++;
            if (ParsePIDProfile(&pFscProfileInfo->ProfileInfo[i], pProfileItem) != 0) goto cleanup;
        }
        else if (strcmp(cString, CJSON_ProfileType_Polynomial) == 0)
        {
            pFscProfileInfo->ProfileInfo[i].ProfileType = FSC_CTL_POLYNOMIAL;
            if (ParsePolynomialProfile(&pFscProfileInfo->ProfileInfo[i], pProfileItem) != 0) goto cleanup;
        }

        ret = FSC_OK;

        // Detailed logging for each parsed profile
        if (verbose > 2)
        {
            FSC_JSON_PROFILE_INFO *P = &pFscProfileInfo->ProfileInfo[i];
            const char *agg = (P->AggregationMode == AGGREGATION_MAX) ? "max" : "average";
            FSCPRINT(" > Profile[%02d]: label='%s', idx=%d, type=%d, aggregation=%s\n",
                     i, P->Label, P->ProfileIndex, P->ProfileType, agg);

            FSCPRINT("   Sensors: count=%d, primary=%d\n", P->SensorCount, P->SensorNum);
            for (int s = 0; s < P->SensorCount; s++)
            {
                FSCPRINT("     sensor_nums[%02d]=%d\n", s, P->SensorNums[s]);
            }

            FSCPRINT("   Power sensors: count=%d\n", P->PowerSensorCount);
            for (int ps = 0; ps < P->PowerSensorCount; ps++)
            {
                FSCPRINT("     power_sensor_nums[%02d]=%d\n", ps, P->PowerSensorNums[ps]);
            }

            if (P->ProfileType == FSC_CTL_PID)
            {
                FSCPRINT("   PID: setpoint=%.3f, type=%d, Kp=%.5f, Ki=%.5f, Kd=%.5f\n",
                         P->PIDParameter.SetPoint,
                         P->PIDParameter.SetPointType,
                         P->PIDParameter.Kp,
                         P->PIDParameter.Ki,
                         P->PIDParameter.Kd);
                FSCPRINT("   PID power buckets: count=%d\n", P->PIDAltCount);
                for (int b = 0; b < P->PIDAltCount; b++)
                {
                    FSC_JSON_PID_POWER_BUCKET *B = &P->PIDAlt[b];
                    FSCPRINT("     bucket[%02d]: power_min=%.3f, power_max=%.3f, Kp=%.5f, Ki=%.5f, Kd=%.5f, setpoint=%.3f, type=%d\n",
                             b, B->PowerMin, B->PowerMax, B->Kp, B->Ki, B->Kd, B->SetPoint, B->SetPointType);
                }
            }
            else if (P->ProfileType == FSC_CTL_POLYNOMIAL)
            {
                FSC_JSON_PROFILE_POLYNOMIAL *Q = &P->PolynomialParameter;
                FSCPRINT("   Polynomial: curve_type=%d, load_scenario=%d\n", Q->CurveType, Q->LoadScenario);
                if (Q->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
                {
                    FSCPRINT("   Coefficients: count=%d\n", Q->CoeffCount);
                    for (int c = 0; c < Q->CoeffCount && c < MAX_POLYNOMIAL_COEFFS; c++)
                    {
                        FSCPRINT("     coeff[%02d]=%.6f\n", c, Q->Coefficients[c]);
                    }
                }
                else if (Q->CurveType == FSC_AMBIENT_CAL_PIECEWISE)
                {
                    FSCPRINT("   Piecewise points: count=%d\n", Q->PointCount);
                    for (int p = 0; p < Q->PointCount && p < MAX_PIECEWISE_POINTS; p++)
                    {
                        FSCPRINT("     point[%02d]: temp=%.3f, pwm=%.3f\n", p, Q->PiecewisePoints[p].temp, Q->PiecewisePoints[p].pwm);
                    }
                }
                FSCPRINT("   Limits: falling_hyst=%.3f, max_rising_rate=%d, max_falling_rate=%d\n",
                         Q->FallingHyst, Q->MaxRisingRate, Q->MaxFallingRate);
            }
        }
    }

    DEBUG_PARSE(verbose, " > Parsed %d profiles\n", pFscProfileInfo->TotalProfileNum);

cleanup:
    FscParseContext_Cleanup(&ctx);
    return ret;
}

/**
 * @fn ParseSystemInfoFromJson
 * @brief Parses the 'system_info' object from a JSON configuration file.
 */
int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose)
{
    FscParseContext ctx = {0};
    cJSON *pSystemInfo = NULL;
    char cString[LABEL_LENGTH_MAX] = {0};
    double dTmp;
    int ret = FSC_ERR_PARSE;

    if (FscParseContext_Init(&ctx, filename) != FSC_OK)
    {
        printf("fsc_parser: Failed to load %s\n", filename);
        return FSC_ERR_IO;
    }

    pSystemInfo = cJSON_GetObjectItem(ctx.json_root, "system_info");
    if (pSystemInfo == NULL) goto cleanup;

    // FSC Mode (optional, defaults to auto)
    if (ConvertcJSONToValue(pSystemInfo, "fsc_mode", cString) != 0 || !strlen(cString))
    {
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_AUTO;
    }
    else if (strcmp(cString, CJSON_FSCMode_Auto) == 0)
    {
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_AUTO;
    }
    else if (strcmp(cString, CJSON_FSCMode_Manual) == 0)
    {
        pFscSystemInfo->FSCMode = FAN_CTL_MODE_MANUAL;
    }

    PARSE_STRING_OR_GOTO(pSystemInfo, "fsc_version", pFscSystemInfo->FSCVersion,
                        sizeof(pFscSystemInfo->FSCVersion), "get fsc_version");

    PARSE_DOUBLE_OR_GOTO(pSystemInfo, "fan_initial_pwm", dTmp, "get fan_initial_pwm");
    pFscSystemInfo->FanInitialPWM = (INT8U)dTmp;

    PARSE_DOUBLE_OR_GOTO(pSystemInfo, "fan_max_pwm", dTmp, "get fan_max_pwm");
    pFscSystemInfo->FanMaxPWM = (INT8U)dTmp;

    PARSE_DOUBLE_OR_GOTO(pSystemInfo, "fan_min_pwm", dTmp, "get fan_min_pwm");
    pFscSystemInfo->FanMinPWM = (INT8U)dTmp;

    ret = FSC_OK;

    DEBUG_PARSE(verbose, " > Parsed system_info: mode=%d, maxPWM=%d%%, minPWM=%d%%, initialPWM=%d%%\n",
               pFscSystemInfo->FSCMode, pFscSystemInfo->FanMaxPWM, pFscSystemInfo->FanMinPWM, pFscSystemInfo->FanInitialPWM);

cleanup:
    FscParseContext_Cleanup(&ctx);
    return ret;
}

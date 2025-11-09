#ifndef FSC_PARSER_H
#define FSC_PARSER_H

#include "Types.h"
#include "fsc.h"
#include "fsc_core.h"

#define CJSON_ProfileType_PID       "pid"
#define CJSON_ProfileType_Polynomial "polynomial"

#define CJSON_FSCMode_Auto          "auto"
#define CJSON_FSCMode_Manual        "manual"

#define LABEL_LENGTH_MAX   32
#define MAX_SENSOR_GROUP_SIZE 64
#define MAX_PID_POWER_BUCKETS 8

// Aggregation modes
#define AGGREGATION_AVERAGE 0
#define AGGREGATION_MAX     1

typedef struct
{
    INT8U   FSCMode;
    char    FSCVersion[16];
    INT8U   FanMaxPWM;
    INT8U   FanMinPWM;
    INT8U   FanInitialPWM;
} PACKED FSC_JSON_SYSTEM_INFO;

typedef struct
{
    double SetPoint;
    signed char   SetPointType;
    double Kp;
    double Ki;
    double Kd;
} PACKED FSC_JSON_PROFILE_PID;

typedef struct
{
    float PowerMin;
    float PowerMax;
    double SetPoint;
    signed char SetPointType;
    double Kp;
    double Ki;
    double Kd;
} PACKED FSC_JSON_PID_POWER_BUCKET;

typedef struct
{
    INT8U CurveType;
    INT8U LoadScenario;
    INT8U CoeffCount;
    float Coefficients[MAX_POLYNOMIAL_COEFFS];
    INT8U PointCount;
    struct {
        INT8U temp;
        INT8U pwm;
    } PiecewisePoints[MAX_PIECEWISE_POINTS];
    float FallingHyst;
    INT8U MaxRisingRate;
    INT8U MaxFallingRate;
} PACKED FSC_JSON_PROFILE_POLYNOMIAL;

typedef struct
{
    char    Label[LABEL_LENGTH_MAX];
    INT8U   ProfileIndex;
    INT8U   SensorNum;
    INT8U   SensorCount;
    INT8U   SensorNums[MAX_SENSOR_GROUP_SIZE];
    INT8U   AggregationMode;
    INT8U   PowerSensorCount;
    INT8U   PowerSensorNums[MAX_SENSOR_GROUP_SIZE];
    INT8U   ProfileType;
    FSC_JSON_PROFILE_PID    PIDParameter;
    INT8U   PIDAltCount;
    FSC_JSON_PID_POWER_BUCKET PIDAlt[MAX_PID_POWER_BUCKETS];
    FSC_JSON_PROFILE_POLYNOMIAL PolynomialParameter;
} PACKED FSC_JSON_PROFILE_INFO;

typedef struct
{
    INT8U   TotalProfileNum;
    INT8U   TotalLinearProfileNum;
    INT8U   TotalPIDProfileNum;
    FSC_JSON_PROFILE_INFO   ProfileInfo[FSC_SENSOR_CNT_MAX];
} PACKED FSC_JSON_ALL_PROFILES_INFO;

extern FSC_JSON_SYSTEM_INFO            g_FscSystemInfo;
extern FSC_JSON_ALL_PROFILES_INFO      g_FscProfileInfo;
extern FSCAmbientCalibration           g_AmbientCalibration;

// Function prototypes
int ParseDebugVerboseFromJson(char *filename, INT8U *verbose);
int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose);
int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose);
int ParseAmbientCalibrationFromJson(char *filename, FSCAmbientCalibration *pAmbientCalibration, INT8U verbose);

#endif // FSC_PARSER_H
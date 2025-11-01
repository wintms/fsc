#ifndef FSC_PARSER_H
#define FSC_PARSER_H

#include "Types.h"
#include "fsc.h"

#define CJSON_ProfileType_Linear    "linear"
#define CJSON_ProfileType_PID       "pid"
#define CJSON_FSCMode_Auto          "auto"
#define CJSON_FSCMode_Manual        "manual"

#define LABEL_LENGTH_MAX   32
#define FSC_SENSOR_CNT_MAX  20

typedef struct
{
    INT8U   ChassisFanMaxNum;
    INT8U   ChassisFanUsedNum;
    INT8U   ChassisFanRotorNum;
    INT8U   ChassisFanRedundantNum;
    INT8U   PSUFanMaxNum;
    INT8U   PSUFanUsedNum;
    INT8U   PSUFanRedundantNum;
    INT8U   SystemMaxFanAirflowNum;
    INT8U   SystemFanAirflow;
    INT8U   FSCMode;
    char    FSCVersion[16];
    INT8U   FanMaxPWM;
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
    INT8U TempMin;
    INT8U TempMax;
    INT8U PwmMin;
    INT8U PwmMax;
    INT8U FallingHyst;
} PACKED FSC_JSON_PROFILE_LINEAR;

typedef struct
{
    char    Label[LABEL_LENGTH_MAX];
    INT8U   ProfileIndex;
    INT8U   SensorNum;
    char    SensorName[16];
    INT8U   ProfileType;
    FSC_JSON_PROFILE_PID    PIDParameter;
    FSC_JSON_PROFILE_LINEAR LinearParameter;
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

extern int ParseDebugVerboseFromJson(char *filename, INT8U *verbose);
extern int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose);
extern int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose);

#endif // FSC_PARSER_H
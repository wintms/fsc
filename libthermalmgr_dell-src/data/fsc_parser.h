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

typedef struct
{
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
    INT8U CurveType;                        // 0=polynomial, 1=piecewise
    INT8U LoadScenario;                     // 0=idle, 1=low_power, 2=full_load
    INT8U CoeffCount;                       // Number of coefficients for polynomial
    float Coefficients[MAX_POLYNOMIAL_COEFFS]; // Polynomial coefficients
    INT8U PointCount;                       // Number of points for piecewise
    struct {
        INT8U temp;                         // Temperature
        INT8U pwm;                          // PWM value
    } PiecewisePoints[MAX_PIECEWISE_POINTS];
    float FallingHyst;                      // Falling hysteresis (default 2)
    INT8U MaxRisingRate;                    // Max rising rate %/cycle (default 10)
    INT8U MaxFallingRate;                   // Max falling rate %/cycle (default 5)
} PACKED FSC_JSON_PROFILE_POLYNOMIAL;

typedef struct
{
    char    Label[LABEL_LENGTH_MAX];
    INT8U   ProfileIndex;
    INT8U   SensorNum;
    char    SensorName[16];
    INT8U   ProfileType;
    FSC_JSON_PROFILE_PID    PIDParameter;
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

extern int ParseDebugVerboseFromJson(char *filename, INT8U *verbose);
int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose);
int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose);
int ParseAmbientCalibrationFromJson(char *filename, FSCAmbientCalibration *pAmbientCalibration, INT8U verbose);

#endif // FSC_PARSER_H
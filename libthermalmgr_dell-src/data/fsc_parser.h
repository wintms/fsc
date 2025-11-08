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
// Maximum number of sensors allowed in a single profile's aggregation group
// Optics may require up to 64 modules
#define MAX_SENSOR_GROUP_SIZE 64
// Maximum PID buckets based on power thresholds
#define MAX_PID_POWER_BUCKETS 8

// Aggregation modes
#define AGGREGATION_AVERAGE 0
#define AGGREGATION_MAX     1

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
    float PowerMin;              // Inclusive lower bound
    float PowerMax;              // Inclusive upper bound
    double SetPoint;             // Optional override; if 0, use default
    signed char SetPointType;    // Optional override; if <0, use default
    double Kp;
    double Ki;
    double Kd;
} PACKED FSC_JSON_PID_POWER_BUCKET;

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
    INT8U   SensorNum;                  // Backward compatibility: first sensor
    INT8U   SensorCount;                // Number of sensors in the group
    INT8U   SensorNums[MAX_SENSOR_GROUP_SIZE]; // Sensor numbers for averaging
    INT8U   AggregationMode;            // 0=average, 1=max
    INT8U   PowerSensorCount;           // Number of power sensors mapped to group
    INT8U   PowerSensorNums[MAX_SENSOR_GROUP_SIZE]; // Power sensor numbers
    char    SensorName[16];
    INT8U   ProfileType;
    FSC_JSON_PROFILE_PID    PIDParameter;
    INT8U   PIDAltCount;                // Number of power buckets
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

extern int ParseDebugVerboseFromJson(char *filename, INT8U *verbose);
int ParseSystemInfoFromJson(char *filename, FSC_JSON_SYSTEM_INFO *pFscSystemInfo, INT8U verbose);
int ParseFSCProfileFromJson(char *filename, FSC_JSON_ALL_PROFILES_INFO *pFscProfileInfo, INT8U verbose);
int ParseAmbientCalibrationFromJson(char *filename, FSCAmbientCalibration *pAmbientCalibration, INT8U verbose);

#endif // FSC_PARSER_H
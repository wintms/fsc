/*************************************************************************
 *
 * fsc_core.c
 * Fan Speed Control core algorithm / functions
 * Refactored version with improved code organization and common utilities
 *
 ************************************************************************/
#include <sys/time.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "Types.h"
#include "OemDefs.h"
#include "fsc_core.h"
#include "fsc.h"
#include "fsc_utils.h"
#include "fsc_math.h"
#include "fsc_common.h"

FSCTempSensor pFSCTempSensorInfo[FSC_PROFILE_MAX_NUM];
FSCAmbientCalibration g_AmbientCalibration;

/*---------------------------------------------------------------------------
* @fn FSCGetAmbientTemperature
*
* @brief This function converts inlet sensor reading to ambient temperature
*        using calibration curve: Ambient = Inlet - ΔT(PWM)
*---------------------------------------------------------------------------*/
float FSCGetAmbientTemperature(INT16S inlet_temp, INT8U last_pwm, INT8U verbose)
{
    float delta_temp = 0.0f;

    if (g_AmbientCalibration.CalType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        delta_temp = FSCMath_EvaluatePolynomial((float)last_pwm,
                                               g_AmbientCalibration.Coefficients,
                                               g_AmbientCalibration.CoeffCount);

        if (verbose > 0)
        {
            FSCPRINT("  Ambient Cal (Polynomial): PWM = %d, ΔT = %.2f\n", last_pwm, delta_temp);
        }
    }
    else if (g_AmbientCalibration.CalType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        delta_temp = FSCMath_InterpolatePiecewise_PwmDeltaT((float)last_pwm,
                                                           g_AmbientCalibration.PiecewisePoints,
                                                           g_AmbientCalibration.PointCount);

        if (verbose > 0)
        {
            FSCPRINT("  Ambient Cal (Piecewise): PWM = %d, ΔT = %.2f\n", last_pwm, delta_temp);
        }
    }

    float ambient_temp = inlet_temp - delta_temp;

    if (verbose > 0)
    {
        FSCPRINT("  Inlet = %d, ΔT = %.2f, Ambient = %.2f\n", inlet_temp, delta_temp, ambient_temp);
    }

    return ambient_temp;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue_PID
*
* @brief This function is used to get fsc pwm using PID algorithm.
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_PID(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    if (0)
    {
        BMCInst = BMCInst;
    }

    FSCPID pPIDInfo;
    float CurrentPWM = 0.0f;
    INT16S p_temp = 0;
    INT16S i_temp = 0;
    INT16S d_temp = 0;

    if (PWMValue == NULL || pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("  Error: NULL pointer in PID calculation\n");
        return FSC_ERR_NULL;
    }

    if (pFSCTempSensorInfo->Present == SENSOR_SCAN_DISABLE)
    {
        if (verbose > 0)
        {
            FSCPRINT("  %-20s is unavailable \n", pFSCTempSensorInfo->Label);
        }
        return FSC_ERR_RANGE;
    }

    pPIDInfo.Pvalue = pFSCTempSensorInfo->fscparam.pidparam.Pvalue;
    pPIDInfo.Ivalue = pFSCTempSensorInfo->fscparam.pidparam.Ivalue;
    pPIDInfo.Dvalue = pFSCTempSensorInfo->fscparam.pidparam.Dvalue;
    pPIDInfo.SetPoint = pFSCTempSensorInfo->fscparam.pidparam.SetPoint;
    pPIDInfo.SetPointType = pFSCTempSensorInfo->fscparam.pidparam.SetPointType;

    if (verbose > 0)
    {
        FSCPRINT("  P = %4.2f, I = %4.2f, D = %4.2f, Setpoint = %d, SetpointType = %d\n",
            pPIDInfo.Pvalue, pPIDInfo.Ivalue, pPIDInfo.Dvalue, pPIDInfo.SetPoint, pPIDInfo.SetPointType );
    }

    p_temp = pFSCTempSensorInfo->CurrentTemp - pFSCTempSensorInfo->LastTemp;
    i_temp = (pFSCTempSensorInfo->CurrentTemp) - (pPIDInfo.SetPoint);
    d_temp = pFSCTempSensorInfo->CurrentTemp - 2 * pFSCTempSensorInfo->LastTemp + pFSCTempSensorInfo->LastLastTemp;

    CurrentPWM = (float)pFSCTempSensorInfo->LastPWM
                 + pPIDInfo.Pvalue * p_temp
                 + pPIDInfo.Ivalue * i_temp
                 + pPIDInfo.Dvalue * d_temp;

    CurrentPWM = FSCMath_ClampValue(CurrentPWM, pFSCTempSensorInfo->MinPWM, pFSCTempSensorInfo->MaxPWM);

    if (verbose > 0)
    {
        FSCPRINT("  Temp = %d, lastTemp = %d, lastlastTemp = %d, PWM = %f, lastPWM = %d\n",
            pFSCTempSensorInfo->CurrentTemp, (int)pFSCTempSensorInfo->LastTemp,
            (int)pFSCTempSensorInfo->LastLastTemp, CurrentPWM, pFSCTempSensorInfo->LastPWM);
    }

    pFSCTempSensorInfo->LastPWM = (INT8U)roundf(CurrentPWM);
    pFSCTempSensorInfo->LastLastTemp = pFSCTempSensorInfo->LastTemp;
    pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;

    *PWMValue = (INT8U)round(CurrentPWM);

    if (verbose > 0)
    {
        FSCPRINT("  %-20s fan PWM output(%%) = %d\n", pFSCTempSensorInfo->Label, *PWMValue);
    }

    return FSC_OK;
}

/*---------------------------------------------------------------------------
* @fn FSCCalculatePolynomialPWM
*
* @brief Calculates PWM from ambient temperature using polynomial curve
*---------------------------------------------------------------------------*/
static INT16S FSCCalculatePolynomialPWM(const FSCPolynomial *pPoly, float ambient_temp, INT8U verbose)
{
    if (0)
    {
        verbose = verbose;
    }
    INT16S pwm;

    if (pPoly->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        float target_pwm = FSCMath_EvaluatePolynomial(ambient_temp,
                                                     pPoly->Coefficients,
                                                     pPoly->CoeffCount);
        pwm = (INT16S)target_pwm;
    }
    else
    {
        pwm = (INT16S)FSCMath_InterpolatePiecewise_TempPwm(ambient_temp,
                                                            pPoly->PiecewisePoints,
                                                            pPoly->PointCount);
    }

    return pwm;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue_Polynomial
*
* @brief This function calculates PWM using ambient base curve algorithm
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_Polynomial(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    if (0)
    {
        BMCInst = BMCInst;
    }

    FSCPolynomial *pPoly;
    float ambient_temp;
    INT16S CurrentPWM;

    if (PWMValue == NULL || pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("  Error: NULL pointer in polynomial calculation\n");
        return FSC_ERR_NULL;
    }

    if (pFSCTempSensorInfo->Present == SENSOR_SCAN_DISABLE)
    {
        if (verbose > 0)
        {
            FSCPRINT("  Polynomial - %-20s is unavailable \n", pFSCTempSensorInfo->Label);
        }
        return FSC_ERR_RANGE;
    }

    pPoly = &(pFSCTempSensorInfo->fscparam.ambientbaseparam);

    // Convert inlet temperature to ambient temperature
    ambient_temp = FSCGetAmbientTemperature(pFSCTempSensorInfo->CurrentTemp,
                                           pFSCTempSensorInfo->LastPWM, verbose);

    // Filter dirty data
    if (abs(pFSCTempSensorInfo->CurrentTemp - pFSCTempSensorInfo->LastTemp) > TEMP_READING_RANGE)
    {
        if (verbose > 0)
        {
            FSCPRINT("  %-20s value change too large, ignoring\n", pFSCTempSensorInfo->Label);
        }
        pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;
        *PWMValue = pFSCTempSensorInfo->LastPWM;
        return FSC_OK;
    }

    // Calculate target PWM
    CurrentPWM = FSCCalculatePolynomialPWM(pPoly, ambient_temp, verbose);

    // Apply hysteresis for temperature declining
    if (ambient_temp < (pFSCTempSensorInfo->LastTemp - g_AmbientCalibration.PiecewisePoints[0].delta_temp))
    {
        float hyst_ambient = ambient_temp + pPoly->FallingHyst;
        INT16S hyst_pwm = FSCCalculatePolynomialPWM(pPoly, hyst_ambient, verbose);

        if (hyst_pwm > CurrentPWM)
        {
            CurrentPWM = hyst_pwm;
        }

        if (verbose > 0)
        {
            FSCPRINT("  Ambient Temp Declining with Hysteresis\n");
        }
    }

    // Apply rate limiting
    CurrentPWM = FSCMath_ApplyRateLimit((float)pFSCTempSensorInfo->LastPWM, (float)CurrentPWM,
                                       pPoly->MaxRisingRate, pPoly->MaxFallingRate);

    // Clamp to boundaries
    CurrentPWM = FSCMath_ClampValue(CurrentPWM, pFSCTempSensorInfo->MinPWM, pFSCTempSensorInfo->MaxPWM);

    if (verbose > 0)
    {
        FSCPRINT("  Inlet = %d, Ambient = %.1f, PWM = %d\n",
            pFSCTempSensorInfo->CurrentTemp, ambient_temp, (INT8U)CurrentPWM);
    }

    pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;
    pFSCTempSensorInfo->LastPWM = (INT8U)round(CurrentPWM);

    *PWMValue = (INT8U)round(CurrentPWM);

    if (verbose > 0)
    {
        FSCPRINT("  %-20s fan PWM output(%%) = %d\n", pFSCTempSensorInfo->Label, *PWMValue);
    }

    return FSC_OK;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue
*
* @brief Main dispatcher for PWM calculation based on algorithm type
*---------------------------------------------------------------------------*/
int FSCGetPWMValue(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    int ret;

    if (pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("Error: NULL pointer in PWM calculation\n");
        return FSC_ERR_NULL;
    }

    switch (pFSCTempSensorInfo->Algorithm)
    {
        case FSC_CTL_PID:
            if (verbose > 0)
            {
                printf("\n");
                FSCPRINT("> PID - %-20s\n", pFSCTempSensorInfo->Label);
            }
            ret = FSCGetPWMValue_PID(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
            break;

        case FSC_CTL_POLYNOMIAL:
            if (verbose > 0)
            {
                printf("\n");
                FSCPRINT("> Polynomial - %-20s\n", pFSCTempSensorInfo->Label);
            }
            ret = FSCGetPWMValue_Polynomial(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
            break;

        default:
            FSCPRINT("Error: Invalid algorithm type %d\n", pFSCTempSensorInfo->Algorithm);
            return FSC_ERR_RANGE;
    }

    return ret;
}

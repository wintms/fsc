/*************************************************************************
 *
 * fsc_core.c
 * Fan Speed Control core algorithm / functions
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

FSCTempSensor pFSCTempSensorInfo[FSC_SENSOR_CNT_MAX];
FSCAmbientCalibration g_AmbientCalibration;

/* -----------------------
 * Internal helpers
 * ----------------------- */
static inline float eval_poly(float base, const float *coeffs, int count)
{
    float acc = 0.0f;
    float power = 1.0f;
    for (int i = 0; i < count && i < MAX_POLYNOMIAL_COEFFS; i++)
    {
        acc += coeffs[i] * power;
        power *= base;
    }
    return acc;
}

static inline INT16S clamp16(INT16S v, INT16S minv, INT16S maxv)
{
    if (v < minv) return minv;
    if (v > maxv) return maxv;
    return v;
}

static inline float ambient_delta_piecewise(const FSCAmbientCalibration *cal, INT8U pwm)
{
    if (!cal || cal->PointCount < 1) return 0.0f;
    if (cal->PointCount == 1) return cal->PiecewisePoints[0].delta_temp;

    if (pwm <= cal->PiecewisePoints[0].pwm)
        return cal->PiecewisePoints[0].delta_temp;
    if (pwm >= cal->PiecewisePoints[cal->PointCount - 1].pwm)
        return cal->PiecewisePoints[cal->PointCount - 1].delta_temp;

    for (int i = 0; i < cal->PointCount - 1; i++)
    {
        INT8U x1 = cal->PiecewisePoints[i].pwm;
        INT8U x2 = cal->PiecewisePoints[i+1].pwm;
        if (pwm >= x1 && pwm <= x2)
        {
            float y1 = cal->PiecewisePoints[i].delta_temp;
            float y2 = cal->PiecewisePoints[i+1].delta_temp;
            return y1 + (y2 - y1) * (pwm - x1) / (float)(x2 - x1);
        }
    }
    return 0.0f;
}

static inline INT16S pwm_piecewise_for_temp(const FSCPolynomial *poly, INT8U temp)
{
    if (!poly || poly->PointCount < 1) return 0;
    if (poly->PointCount == 1) return poly->PiecewisePoints[0].pwm;

    if (temp <= poly->PiecewisePoints[0].temp)
        return poly->PiecewisePoints[0].pwm;
    if (temp >= poly->PiecewisePoints[poly->PointCount - 1].temp)
        return poly->PiecewisePoints[poly->PointCount - 1].pwm;

    for (int i = 0; i < poly->PointCount - 1; i++)
    {
        INT8U t1 = poly->PiecewisePoints[i].temp;
        INT8U t2 = poly->PiecewisePoints[i+1].temp;
        if (temp >= t1 && temp <= t2)
        {
            INT8U p1 = poly->PiecewisePoints[i].pwm;
            INT8U p2 = poly->PiecewisePoints[i+1].pwm;
            return (INT16S)(p1 + (p2 - p1) * (temp - t1) / (INT16S)(t2 - t1));
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue_PID
*
* @brief This function is used to get fsc pwm using PID algorithm.
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_PID(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    UN_USED(BMCInst);

    FSCPID pPIDInfo;

    float CurrentPWM = 0.0;
    INT16S p_temp = 0.0;
    INT16S i_temp = 0.0;
    INT16S d_temp = 0.0;

    if(PWMValue == NULL || pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("pointer can't be NULL \n");
        return -1;
    }

    if(pFSCTempSensorInfo->Present == SENSOR_SCAN_DISABLE)
    {
        if(verbose > 1)
        {
            FSCPRINT(" > PID - %-20s is disabled \n", pFSCTempSensorInfo->Label);
        }

        return -1;
    }

    pPIDInfo.Pvalue = pFSCTempSensorInfo->fscparam.pidparam.Pvalue;
    pPIDInfo.Ivalue = pFSCTempSensorInfo->fscparam.pidparam.Ivalue;
    pPIDInfo.Dvalue = pFSCTempSensorInfo->fscparam.pidparam.Dvalue;
    pPIDInfo.SetPoint = pFSCTempSensorInfo->fscparam.pidparam.SetPoint;
    pPIDInfo.SetPointType = pFSCTempSensorInfo->fscparam.pidparam.SetPointType;

    if(verbose > 1)
    {
        FSCPRINT(" > P = %4.2f, I = %4.2f, D = %4.2f, Setpoint = %d, SetpointType = %d\n",
                         pPIDInfo.Pvalue, pPIDInfo.Ivalue, pPIDInfo.Dvalue, pPIDInfo.SetPoint, pPIDInfo.SetPointType );
    }

    p_temp = pFSCTempSensorInfo->CurrentTemp - pFSCTempSensorInfo->LastTemp;
    i_temp = (pFSCTempSensorInfo->CurrentTemp) - (pPIDInfo.SetPoint);
    d_temp = pFSCTempSensorInfo->CurrentTemp - 2 * pFSCTempSensorInfo->LastTemp + pFSCTempSensorInfo->LastLastTemp;

    CurrentPWM = pFSCTempSensorInfo->LastPWM
                 + pPIDInfo.Pvalue * p_temp
                 + pPIDInfo.Ivalue * i_temp
                 + pPIDInfo.Dvalue * d_temp;


    if(verbose > 1)
    {
        FSCPRINT(" > %-20s = %4d, lastTemp = %4d, lastlastTemp = %4d, currentPWM = %d,lastPWM = %02d\n",
                         pFSCTempSensorInfo->Label, pFSCTempSensorInfo->CurrentTemp, (int)pFSCTempSensorInfo->LastTemp,
                         (int)pFSCTempSensorInfo->LastLastTemp,(int)CurrentPWM,(int)(pFSCTempSensorInfo->LastPWM));
    }

    if(CurrentPWM > pFSCTempSensorInfo->MaxPWM)
        CurrentPWM = pFSCTempSensorInfo->MaxPWM;

    if(CurrentPWM < pFSCTempSensorInfo->MinPWM)
        CurrentPWM = pFSCTempSensorInfo->MinPWM;

    if(verbose > 0)
    {
        FSCPRINT("%-20s = %4d, currentPWM = %2d\n",
                         pFSCTempSensorInfo->Label, pFSCTempSensorInfo->CurrentTemp, (int)CurrentPWM);
    }

    pFSCTempSensorInfo->LastPWM = CurrentPWM;
    pFSCTempSensorInfo->LastLastTemp = pFSCTempSensorInfo->LastTemp;
    pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;

    *PWMValue = round(CurrentPWM);

    return 0;

}


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
        delta_temp = eval_poly((float)last_pwm, g_AmbientCalibration.Coefficients, g_AmbientCalibration.CoeffCount);
        if (verbose > 1)
        {
            FSCPRINT(" > Ambient Cal (Polynomial): PWM=%d, ΔT=%.2f\n", last_pwm, delta_temp);
        }
    }
    else if (g_AmbientCalibration.CalType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        delta_temp = ambient_delta_piecewise(&g_AmbientCalibration, last_pwm);
        if (verbose > 1)
        {
            FSCPRINT(" > Ambient Cal (Piecewise): PWM=%d, ΔT=%.2f\n", last_pwm, delta_temp);
        }
    }

    float ambient_temp = inlet_temp - delta_temp;
    
    if (verbose > 0)
    {
        FSCPRINT("Inlet=%d, ΔT=%.2f, Ambient=%.2f\n", inlet_temp, delta_temp, ambient_temp);
    }
    
    return ambient_temp;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue_Polynomial
*
* @brief This function calculates PWM using ambient base curve algorithm
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_Polynomial(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    UN_USED(BMCInst);
    
    FSCPolynomial *pPolynomial = NULL;
    INT16S CurrentPWM = 0;
    float ambient_temp = 0.0;
    int i;
    
    if(PWMValue == NULL || pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("pointer can't be NULL \n");
        return -1;
    }

    if(pFSCTempSensorInfo->Present == SENSOR_SCAN_DISABLE)
    {
        if(verbose > 1)
        {
            FSCPRINT(" > Polynomial - %-20s is disabled \n", pFSCTempSensorInfo->Label);
        }
        return -1;
    }
    
    pPolynomial = &(pFSCTempSensorInfo->fscparam.ambientbaseparam);
    
    // Convert inlet temperature to ambient temperature using calibration
    ambient_temp = FSCGetAmbientTemperature(pFSCTempSensorInfo->CurrentTemp, 
                                          pFSCTempSensorInfo->LastPWM, verbose);
    
    // Filter dirty data - same as linear algorithm
    if(abs(pFSCTempSensorInfo->CurrentTemp - pFSCTempSensorInfo->LastTemp) > TEMP_READING_RANGE)
    {
        if(verbose > 1)
        {
            FSCPRINT(" > %-20s = %4d, lastTemp = %4d, currentPWM = %02d, lastPWM = %02d - Reading change large than %d, ignore\n",
                             pFSCTempSensorInfo->Label, (int)pFSCTempSensorInfo->CurrentTemp, (int)pFSCTempSensorInfo->LastTemp,
                             (int)CurrentPWM, (int)pFSCTempSensorInfo->LastPWM, TEMP_READING_RANGE);
        }

        pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;
        *PWMValue = pFSCTempSensorInfo->LastPWM;
        return 0;
    }
    
    // Calculate target PWM based on ambient base curve
    if (pPolynomial->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        CurrentPWM = (INT16S)eval_poly(ambient_temp, pPolynomial->Coefficients, pPolynomial->CoeffCount);
    }
    else
    {
        INT8U ambient_temp_int = (INT8U)ambient_temp;
        if (pPolynomial->PointCount < 2) {
            CurrentPWM = pFSCTempSensorInfo->LastPWM;
        } else {
            CurrentPWM = pwm_piecewise_for_temp(pPolynomial, ambient_temp_int);
        }
    }
    
    // Apply hysteresis for temperature declining
    if(ambient_temp < (pFSCTempSensorInfo->LastTemp - g_AmbientCalibration.PiecewisePoints[0].delta_temp))
    {
        // Add falling hysteresis
        float hyst_ambient = ambient_temp + pPolynomial->FallingHyst;
        // Recalculate PWM with hysteresis for polynomial curve
        if (pPolynomial->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
        {
            INT16S hyst_pwm = (INT16S)eval_poly(hyst_ambient, pPolynomial->Coefficients, pPolynomial->CoeffCount);
            if (hyst_pwm > CurrentPWM) {
                CurrentPWM = hyst_pwm;
            }
        }
        
        if(verbose > 1)
        {
            FSCPRINT(" > PWM: %d, Ambient Temp Declining with Hysteresis\n", CurrentPWM);
        }
    }
    
    // Apply rate limiting
    INT16S pwm_diff = CurrentPWM - pFSCTempSensorInfo->LastPWM;
    INT16S max_change = 0;
    
    if (pwm_diff > 0)
    {
        // Rising rate limit
        max_change = pPolynomial->MaxRisingRate;
    }
    else if (pwm_diff < 0)
    {
        // Falling rate limit
        max_change = pPolynomial->MaxFallingRate;
        pwm_diff = -pwm_diff; // Make positive for comparison
    }
    
    if (pwm_diff > max_change)
    {
        if (CurrentPWM > pFSCTempSensorInfo->LastPWM)
        {
            CurrentPWM = pFSCTempSensorInfo->LastPWM + max_change;
        }
        else
        {
            CurrentPWM = pFSCTempSensorInfo->LastPWM - max_change;
        }
        
        if(verbose > 1)
        {
            FSCPRINT(" > PWM rate limited: %d\n", CurrentPWM);
        }
    }
    
    // Boundary clamping
    CurrentPWM = clamp16(CurrentPWM, (INT16S)pFSCTempSensorInfo->MinPWM, (INT16S)pFSCTempSensorInfo->MaxPWM);
    
    if(verbose > 0)
    {
        FSCPRINT("%-20s Ambient=%.1f, currentPWM = %2d\n",
                         pFSCTempSensorInfo->Label, ambient_temp, (INT8U)CurrentPWM);
    }
    
    pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;
    pFSCTempSensorInfo->LastPWM = (INT8U)CurrentPWM;

    *PWMValue = (INT8U)CurrentPWM;

    return 0;
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue_Polynomial
*
* @brief Compatibility wrapper for ambient-base algorithm
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_Polynomial(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    return FSCGetPWMValue_Polynomial(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
}

/*---------------------------------------------------------------------------
* @fn FSCGetPWMValue
*
* @brief This function is used to get fsc pwm.
*---------------------------------------------------------------------------*/
int FSCGetPWMValue(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    UN_USED(BMCInst);

    int ret = -1;

    if(pFSCTempSensorInfo != NULL)
    {

        switch(pFSCTempSensorInfo->Algorithm)
        {
            case FSC_CTL_PID:

                ret = FSCGetPWMValue_PID(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
                if(verbose > 2)
                {
                    FSCPRINT("  >> Get PID    PWM ret: %d\n", ret);
                }

                return ret;

            case FSC_CTL_POLYNOMIAL:

                ret = FSCGetPWMValue_Polynomial(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
                if(verbose > 2)
                {
                    FSCPRINT("  >> Get POLYNOMIAL PWM ret: %d\n", ret);
                }

                return ret;
                
            default:
                FSCPRINT("Invalid Cooling algorithm. \n");
                return -1;
        }
    }
    else
    {
        FSCPRINT("pointer can't be NULL. \n");
        return -1;
    }

    return 0;
}



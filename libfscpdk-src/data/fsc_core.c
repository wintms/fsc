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
* @fn FSCGetPWMValue_Linear
*
* @brief This function is used to get fsc pwm using linear algorithm.
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_Linear(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{

    UN_USED(BMCInst);

    FSCLinear *pLinearInfo = NULL;
    INT16S CurrentPWM = 0;

    if(PWMValue == NULL || pFSCTempSensorInfo == NULL)
    {
        FSCPRINT("pointer can't be NULL \n");
        return -1;
    }

    if(pFSCTempSensorInfo->Present == SENSOR_SCAN_DISABLE)
    {
        if(verbose > 1)
        {
            FSCPRINT(" > Linear - %-20s is disabled \n", pFSCTempSensorInfo->Label);
        }

        return -1;
    }
    
    pLinearInfo = &(pFSCTempSensorInfo->fscparam.linearparam);

    if(verbose > 1)
    {
        FSCPRINT(" > TempMin = %d, TempMax = %d, PwmMin = %d, PwmMax = %d, FallingHyst = %d\n",
                pLinearInfo->TempMin, pLinearInfo->TempMax, pLinearInfo->PwmMin, pLinearInfo->PwmMax, pLinearInfo->FallingHyst);
    }
    
    // Filter dirty data
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
        CurrentPWM = (INT16S)(*PWMValue);

        if(verbose > 1)
        {
            FSCPRINT(" > %-20s = %4d, lastTemp = %4d, currentPWM = %02d, lastPWM = %02d - Errata\n",
                             pFSCTempSensorInfo->Label, pFSCTempSensorInfo->CurrentTemp, (int)pFSCTempSensorInfo->LastTemp,
                             (int)CurrentPWM, (int)(pFSCTempSensorInfo->LastPWM));
        }

        return 0; // treat it as normal.
    }

    if(pFSCTempSensorInfo->CurrentTemp > pFSCTempSensorInfo->LastTemp)
    {
        CurrentPWM = (INT16S)(((pLinearInfo->PwmMax - pLinearInfo->PwmMin) / (pLinearInfo->TempMax - pLinearInfo->TempMin))
                        * (pFSCTempSensorInfo->CurrentTemp - pLinearInfo->TempMin)
                        + pLinearInfo->PwmMin);
        if (CurrentPWM < (INT16S)pFSCTempSensorInfo->LastPWM)
        {
            CurrentPWM = (INT16S)pFSCTempSensorInfo->LastPWM;
        }
        
        if(verbose > 1)
        {
            FSCPRINT(" > PWM: %d, Temp Rising\n", CurrentPWM);
        }
    }
    else if(pFSCTempSensorInfo->CurrentTemp < pFSCTempSensorInfo->LastTemp) //Temp Declining
    {
        CurrentPWM = (INT16S)(((pLinearInfo->PwmMax - pLinearInfo->PwmMin) / (pLinearInfo->TempMax - pLinearInfo->TempMin))
                        * (pFSCTempSensorInfo->CurrentTemp - pLinearInfo->TempMin + pLinearInfo->FallingHyst)
                        + pLinearInfo->PwmMin);
        if (CurrentPWM > (INT16S)pFSCTempSensorInfo->LastPWM)
        {
            CurrentPWM = (INT16S)pFSCTempSensorInfo->LastPWM;
        }
        
        if(verbose > 1)
        {
            FSCPRINT(" > PWM: %d, Temp Declining\n", CurrentPWM);
        }
    }
    else // Temperature and altitude no change
    {
        CurrentPWM = (INT16S)pFSCTempSensorInfo->LastPWM;
    }
    
    if (CurrentPWM < 0)
        CurrentPWM = (INT16S)pLinearInfo->PwmMin;

    if(verbose > 1)
    {
        FSCPRINT(" > %-20s = %4d, lastTemp = %4d, currentPWM = %02d, lastPWM = %02d\n",
                         pFSCTempSensorInfo->Label, pFSCTempSensorInfo->CurrentTemp, (int)pFSCTempSensorInfo->LastTemp,
                         (int)CurrentPWM, (int)(pFSCTempSensorInfo->LastPWM));
    }
    
    if(CurrentPWM > (INT16S)pLinearInfo->PwmMax)
        CurrentPWM = (INT16S)pLinearInfo->PwmMax;

    if(CurrentPWM < (INT16S)pLinearInfo->PwmMin)
        CurrentPWM = (INT16S)pLinearInfo->PwmMin;
    
    if(verbose > 0)
    {
        FSCPRINT("%-20s = %4d, currentPWM = %2d\n",
                         pFSCTempSensorInfo->Label, pFSCTempSensorInfo->CurrentTemp, (INT8U)CurrentPWM);
    }
    
    pFSCTempSensorInfo->LastTemp = pFSCTempSensorInfo->CurrentTemp;
    pFSCTempSensorInfo->LastPWM = (INT8U)CurrentPWM;

    *PWMValue = (INT8U)CurrentPWM;

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
    float delta_temp = 0.0;
    int i;
    
    if (g_AmbientCalibration.CalType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        // Polynomial calculation: ΔT = a0 + a1*PWM + a2*PWM^2 + a3*PWM^3
        float pwm_power = 1.0;
        for (i = 0; i < g_AmbientCalibration.CoeffCount && i < MAX_POLYNOMIAL_COEFFS; i++)
        {
            delta_temp += g_AmbientCalibration.Coefficients[i] * pwm_power;
            pwm_power *= last_pwm;
        }
        
        if (verbose > 1)
        {
            FSCPRINT(" > Ambient Cal (Polynomial): PWM=%d, ΔT=%.2f\n", last_pwm, delta_temp);
        }
    }
    else if (g_AmbientCalibration.CalType == FSC_AMBIENT_CAL_PIECEWISE)
    {
        // Piecewise linear interpolation
        if (g_AmbientCalibration.PointCount < 2)
        {
            delta_temp = 0.0; // No calibration data
        }
        else if (last_pwm <= g_AmbientCalibration.PiecewisePoints[0].pwm)
        {
            delta_temp = g_AmbientCalibration.PiecewisePoints[0].delta_temp;
        }
        else if (last_pwm >= g_AmbientCalibration.PiecewisePoints[g_AmbientCalibration.PointCount-1].pwm)
        {
            delta_temp = g_AmbientCalibration.PiecewisePoints[g_AmbientCalibration.PointCount-1].delta_temp;
        }
        else
        {
            // Linear interpolation between two points
            for (i = 0; i < g_AmbientCalibration.PointCount - 1; i++)
            {
                if (last_pwm >= g_AmbientCalibration.PiecewisePoints[i].pwm && 
                    last_pwm <= g_AmbientCalibration.PiecewisePoints[i+1].pwm)
                {
                    float x1 = g_AmbientCalibration.PiecewisePoints[i].pwm;
                    float y1 = g_AmbientCalibration.PiecewisePoints[i].delta_temp;
                    float x2 = g_AmbientCalibration.PiecewisePoints[i+1].pwm;
                    float y2 = g_AmbientCalibration.PiecewisePoints[i+1].delta_temp;
                    
                    delta_temp = y1 + (y2 - y1) * (last_pwm - x1) / (x2 - x1);
                    break;
                }
            }
        }
        
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
* @fn FSCGetPWMValue_AmbientBase
*
* @brief This function calculates PWM using ambient base curve algorithm
*---------------------------------------------------------------------------*/
int FSCGetPWMValue_AmbientBase(INT8U *PWMValue, FSCTempSensor *pFSCTempSensorInfo, INT8U verbose, int BMCInst)
{
    UN_USED(BMCInst);
    
    FSCAmbientBase *pAmbientBase = NULL;
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
            FSCPRINT(" > AmbientBase - %-20s is disabled \n", pFSCTempSensorInfo->Label);
        }
        return -1;
    }
    
    pAmbientBase = &(pFSCTempSensorInfo->fscparam.ambientbaseparam);
    
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
    if (pAmbientBase->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
    {
        // Polynomial calculation: PWM = a0 + a1*T + a2*T^2 + a3*T^3
        float temp_power = 1.0;
        float target_pwm = 0.0;
        for (i = 0; i < pAmbientBase->CoeffCount && i < MAX_POLYNOMIAL_COEFFS; i++)
        {
            target_pwm += pAmbientBase->Coefficients[i] * temp_power;
            temp_power *= ambient_temp;
        }
        CurrentPWM = (INT16S)target_pwm;
    }
    else
    {
        // Piecewise linear interpolation
        INT8U ambient_temp_int = (INT8U)ambient_temp;
        
        if (pAmbientBase->PointCount < 2)
        {
            CurrentPWM = pFSCTempSensorInfo->LastPWM; // No curve data, keep last PWM
        }
        else if (ambient_temp_int <= pAmbientBase->PiecewisePoints[0].temp)
        {
            CurrentPWM = pAmbientBase->PiecewisePoints[0].pwm;
        }
        else if (ambient_temp_int >= pAmbientBase->PiecewisePoints[pAmbientBase->PointCount-1].temp)
        {
            CurrentPWM = pAmbientBase->PiecewisePoints[pAmbientBase->PointCount-1].pwm;
        }
        else
        {
            // Linear interpolation between two points
            for (i = 0; i < pAmbientBase->PointCount - 1; i++)
            {
                if (ambient_temp_int >= pAmbientBase->PiecewisePoints[i].temp && 
                    ambient_temp_int <= pAmbientBase->PiecewisePoints[i+1].temp)
                {
                    INT8U t1 = pAmbientBase->PiecewisePoints[i].temp;
                    INT8U p1 = pAmbientBase->PiecewisePoints[i].pwm;
                    INT8U t2 = pAmbientBase->PiecewisePoints[i+1].temp;
                    INT8U p2 = pAmbientBase->PiecewisePoints[i+1].pwm;
                    
                    CurrentPWM = p1 + (p2 - p1) * (ambient_temp_int - t1) / (t2 - t1);
                    break;
                }
            }
        }
    }
    
    // Apply hysteresis for temperature declining
    if(ambient_temp < (pFSCTempSensorInfo->LastTemp - g_AmbientCalibration.PiecewisePoints[0].delta_temp))
    {
        // Add falling hysteresis
        float hyst_ambient = ambient_temp + pAmbientBase->FallingHyst;
        
        // Recalculate PWM with hysteresis
        if (pAmbientBase->CurveType == FSC_AMBIENT_CAL_POLYNOMIAL)
        {
            float temp_power = 1.0;
            float target_pwm = 0.0;
            for (i = 0; i < pAmbientBase->CoeffCount && i < MAX_POLYNOMIAL_COEFFS; i++)
            {
                target_pwm += pAmbientBase->Coefficients[i] * temp_power;
                temp_power *= hyst_ambient;
            }
            INT16S hyst_pwm = (INT16S)target_pwm;
            
            if (hyst_pwm > CurrentPWM)
            {
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
        max_change = pAmbientBase->MaxRisingRate;
    }
    else if (pwm_diff < 0)
    {
        // Falling rate limit
        max_change = pAmbientBase->MaxFallingRate;
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
    if(CurrentPWM > (INT16S)pFSCTempSensorInfo->MaxPWM)
        CurrentPWM = (INT16S)pFSCTempSensorInfo->MaxPWM;

    if(CurrentPWM < (INT16S)pFSCTempSensorInfo->MinPWM)
        CurrentPWM = (INT16S)pFSCTempSensorInfo->MinPWM;
    
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

            case FSC_CTL_LINEAR:

                ret = FSCGetPWMValue_Linear(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
                if(verbose > 2)
                {
                    FSCPRINT("  >> Get LINEAR PWM ret: %d\n", ret);
                }

                return ret;
                
            case FSC_CTL_AMBIENT_BASE:

                ret = FSCGetPWMValue_AmbientBase(PWMValue, pFSCTempSensorInfo, verbose, BMCInst);
                if(verbose > 2)
                {
                    FSCPRINT("  >> Get AMBIENT_BASE PWM ret: %d\n", ret);
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



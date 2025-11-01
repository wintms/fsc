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



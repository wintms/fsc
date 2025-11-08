/*************************************************************************
 *
 * fsc_math.c
 * Mathematical utility functions for Fan Speed Control
 * Common calculations: polynomial evaluation, piecewise interpolation
 *
 ************************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include "Types.h"
#include "fsc_math.h"

/*---------------------------------------------------------------------------
* @fn FSCMath_EvaluatePolynomial
*
* @brief Evaluates a polynomial function: result = sum(coeffs[i] * x^i)
* @param[in] x Input value
* @param[in] coeffs Coefficient array
* @param[in] coeff_count Number of coefficients
* @return Calculated result
*---------------------------------------------------------------------------*/
float FSCMath_EvaluatePolynomial(float x, const float *coeffs, int coeff_count)
{
    float result = 0.0f;
    float x_power = 1.0f;
    int i;

    if (coeffs == NULL || coeff_count <= 0)
    {
        return 0.0f;
    }

    for (i = 0; i < coeff_count; i++)
    {
        result += coeffs[i] * x_power;
        x_power *= x;
    }

    return result;
}

/*---------------------------------------------------------------------------
* @fn FSCMath_ClampValue
*
* @brief Clamps a value between min and max boundaries
* @param[in] value Value to clamp
* @param[in] min Minimum boundary
* @param[in] max Maximum boundary
* @return Clamped value
*---------------------------------------------------------------------------*/
float FSCMath_ClampValue(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

/*---------------------------------------------------------------------------
* @fn FSCMath_ApplyRateLimit
*
* @brief Applies rate limiting to a value change
* @param[in] current Current value
* @param[in] target Target value
* @param[in] max_rising Max rising rate per cycle
* @param[in] max_falling Max falling rate per cycle
* @return Rate-limited value
*---------------------------------------------------------------------------*/
float FSCMath_ApplyRateLimit(float current, float target,
                             float max_rising, float max_falling)
{
    float diff = target - current;
    float max_change;

    if (diff > 0)
    {
        max_change = max_rising;
    }
    else if (diff < 0)
    {
        max_change = -max_falling;
    }
    else
    {
        return current;
    }

    if (fabs(diff) > fabs(max_change))
    {
        return current + max_change;
    }

    return target;
}

/*---------------------------------------------------------------------------
* @fn FSCMath_InterpolatePiecewise
*
* @brief Performs linear interpolation in a piecewise table
* @param[in] x Input x value
* @param[in] points Array of data points
* @param[in] point_count Number of points
* @param[in] x_field_offset Offset of x field in point struct
* @param[in] y_field_offset Offset of y field in point struct
* @param[in] point_size Size of each point struct
* @return Interpolated y value
*---------------------------------------------------------------------------*/
float FSCMath_InterpolatePiecewise(float x, const void *points, int point_count,
                                   size_t x_field_offset, size_t y_field_offset,
                                   size_t point_size)
{
    const unsigned char *point_data = (const unsigned char *)points;
    float x1, y1, x2, y2;
    float *x_ptr, *y_ptr;
    int i;

    if (points == NULL || point_count < 2)
    {
        return 0.0f;
    }

    // Get x value from first point
    x_ptr = (float *)(point_data + x_field_offset);

    // Below lower bound
    if (x <= *x_ptr)
    {
        y_ptr = (float *)(point_data + y_field_offset);
        return *y_ptr;
    }

    // Above upper bound
    x_ptr = (float *)(point_data + (point_count - 1) * point_size + x_field_offset);
    if (x >= *x_ptr)
    {
        y_ptr = (float *)(point_data + (point_count - 1) * point_size + y_field_offset);
        return *y_ptr;
    }

    // Linear interpolation
    for (i = 0; i < point_count - 1; i++)
    {
        x_ptr = (float *)(point_data + i * point_size + x_field_offset);
        x1 = *x_ptr;

        x_ptr = (float *)(point_data + (i + 1) * point_size + x_field_offset);
        x2 = *x_ptr;

        if (x >= x1 && x <= x2)
        {
            y_ptr = (float *)(point_data + i * point_size + y_field_offset);
            y1 = *y_ptr;

            y_ptr = (float *)(point_data + (i + 1) * point_size + y_field_offset);
            y2 = *y_ptr;

            // Linear interpolation: y = y1 + (y2 - y1) * (x - x1) / (x2 - x1)
            return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
        }
    }

    return 0.0f;
}

/*---------------------------------------------------------------------------
* @fn FSCMath_InterpolatePiecewise_PwmDeltaT
*
* @brief Specialized version for ambient calibration (PWM -> DeltaT)
* @param[in] pwm PWM value
* @param[in] points Array of FsMath_PwmDeltaT points
* @param[in] point_count Number of points
* @return Interpolated delta temperature
*---------------------------------------------------------------------------*/
float FSCMath_InterpolatePiecewise_PwmDeltaT(float pwm,
                                            const FSCMath_PwmDeltaT *points,
                                            int point_count)
{
    return FSCMath_InterpolatePiecewise(pwm, points, point_count,
                                        offsetof(FSCMath_PwmDeltaT, pwm),
                                        offsetof(FSCMath_PwmDeltaT, delta_temp),
                                        sizeof(FSCMath_PwmDeltaT));
}

/*---------------------------------------------------------------------------
* @fn FSCMath_InterpolatePiecewise_TempPwm
*
* @brief Specialized version for ambient base curve (Temp -> PWM)
* @param[in] temp Temperature value
* @param[in] points Array of FSCMath_TempPwm points
* @param[in] point_count Number of points
* @return Interpolated PWM value
*---------------------------------------------------------------------------*/
float FSCMath_InterpolatePiecewise_TempPwm(float temp,
                                           const FSCMath_TempPwm *points,
                                           int point_count)
{
    return FSCMath_InterpolatePiecewise(temp, points, point_count,
                                        offsetof(FSCMath_TempPwm, temp),
                                        offsetof(FSCMath_TempPwm, pwm),
                                        sizeof(FSCMath_TempPwm));
}

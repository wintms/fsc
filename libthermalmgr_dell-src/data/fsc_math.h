/*************************************************************************
 *
 * fsc_math.h
 * Mathematical utility functions for Fan Speed Control
 * Common calculations: polynomial evaluation, piecewise interpolation
 *
 ************************************************************************/
#ifndef FSC_MATH_H
#define FSC_MATH_H

#include "Types.h"

// Piecewise point structures for different use cases
typedef struct
{
    INT8U pwm;          // PWM value
    float delta_temp;   // Temperature delta
} FSCMath_PwmDeltaT;

typedef struct
{
    INT8U temp;         // Temperature value
    INT8U pwm;          // PWM value
} FSCMath_TempPwm;

// Function prototypes
float FSCMath_EvaluatePolynomial(float x, const float *coeffs, int coeff_count);
float FSCMath_ClampValue(float value, float min, float max);
float FSCMath_ApplyRateLimit(float current, float target,
                             float max_rising, float max_falling);
float FSCMath_InterpolatePiecewise(float x, const void *points, int point_count,
                                   size_t x_field_offset, size_t y_field_offset,
                                   size_t point_size);
float FSCMath_InterpolatePiecewise_PwmDeltaT(float pwm,
                                            const FSCMath_PwmDeltaT *points,
                                            int point_count);
float FSCMath_InterpolatePiecewise_TempPwm(float temp,
                                           const FSCMath_TempPwm *points,
                                           int point_count);

#endif // FSC_MATH_H

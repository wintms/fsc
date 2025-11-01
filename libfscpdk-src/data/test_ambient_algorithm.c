#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Types.h"
#include "fsc_core.h"
#include "fsc_parser.h"

// Test data based on B2F configuration
void test_ambient_calibration()
{
    printf("=== Testing Ambient Calibration ===\n");
    
    // Initialize calibration data (piecewise linear)
    g_AmbientCalibration.CalType = FSC_AMBIENT_CAL_PIECEWISE;
    g_AmbientCalibration.PointCount = 6;
    
    // Test data from PPT
    g_AmbientCalibration.PiecewisePoints[0].pwm = 0;
    g_AmbientCalibration.PiecewisePoints[0].delta_temp = 8.5;
    g_AmbientCalibration.PiecewisePoints[1].pwm = 20;
    g_AmbientCalibration.PiecewisePoints[1].delta_temp = 6.2;
    g_AmbientCalibration.PiecewisePoints[2].pwm = 40;
    g_AmbientCalibration.PiecewisePoints[2].delta_temp = 4.8;
    g_AmbientCalibration.PiecewisePoints[3].pwm = 60;
    g_AmbientCalibration.PiecewisePoints[3].delta_temp = 3.5;
    g_AmbientCalibration.PiecewisePoints[4].pwm = 80;
    g_AmbientCalibration.PiecewisePoints[4].delta_temp = 2.8;
    g_AmbientCalibration.PiecewisePoints[5].pwm = 100;
    g_AmbientCalibration.PiecewisePoints[5].delta_temp = 2.2;
    
    // Test cases
    float test_inlet_temps[] = {35.0, 40.0, 45.0, 50.0};
    INT8U test_pwm_values[] = {0, 30, 60, 100};
    
    printf("Inlet Temperature Calibration Test:\n");
    printf("PWM\tDelta_T\tInlet_Temp\tAmbient_Temp\n");
    
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            float ambient_temp = FSCGetAmbientTemperature((INT16S)test_inlet_temps[i], test_pwm_values[j], 1);
            float delta_t = test_inlet_temps[i] - ambient_temp;
            printf("%d\t%.2f\t%.1f\t\t%.1f\n", 
                   test_pwm_values[j], delta_t, test_inlet_temps[i], ambient_temp);
        }
    }
    printf("\n");
}

void test_ambient_base_curve()
{
    printf("=== Testing Ambient Base Curve ===\n");
    
    // Setup a test sensor with ambient base configuration
    FSCTempSensor test_sensor;
    memset(&test_sensor, 0, sizeof(FSCTempSensor));
    
    test_sensor.Present = SENSOR_SCAN_ENABLE;
    test_sensor.Algorithm = FSC_CTL_AMBIENT_BASE;
    test_sensor.CurrentTemp = 40;  // 40°C ambient temperature
    test_sensor.MinPWM = 30;
    test_sensor.MaxPWM = 100;
    strcpy(test_sensor.Label, "Test_Ambient");
    
    // Initialize ambient base parameters (piecewise linear)
    FSCAmbientBase *ambient_base = &test_sensor.fscparam.ambientbaseparam;
    ambient_base->CurveType = FSC_AMBIENT_CAL_PIECEWISE;
    ambient_base->LoadScenario = 0;  // Normal load
    ambient_base->PointCount = 8;
    ambient_base->FallingHyst = 2;
    ambient_base->MaxRisingRate = 10;
    ambient_base->MaxFallingRate = 5;
    
    // Test data from PPT (B2F ambient base curve)
    ambient_base->PiecewisePoints[0].temp = 25; ambient_base->PiecewisePoints[0].pwm = 30;
    ambient_base->PiecewisePoints[1].temp = 30; ambient_base->PiecewisePoints[1].pwm = 30;
    ambient_base->PiecewisePoints[2].temp = 35; ambient_base->PiecewisePoints[2].pwm = 32;
    ambient_base->PiecewisePoints[3].temp = 40; ambient_base->PiecewisePoints[3].pwm = 38;
    ambient_base->PiecewisePoints[4].temp = 45; ambient_base->PiecewisePoints[4].pwm = 50;
    ambient_base->PiecewisePoints[5].temp = 50; ambient_base->PiecewisePoints[5].pwm = 70;
    ambient_base->PiecewisePoints[6].temp = 55; ambient_base->PiecewisePoints[6].pwm = 85;
    ambient_base->PiecewisePoints[7].temp = 60; ambient_base->PiecewisePoints[7].pwm = 100;
    
    // Test temperature range
    INT16S test_temps[] = {25, 30, 35, 40, 45, 50, 55, 60};
    int num_tests = sizeof(test_temps) / sizeof(test_temps[0]);
    
    printf("Ambient Base Curve Test:\n");
    printf("Ambient_Temp\tCalculated_PWM\n");
    
    for(int i = 0; i < num_tests; i++)
    {
        test_sensor.CurrentTemp = test_temps[i];
        INT8U pwm_result = 0;
        int ret = FSCGetPWMValue_AmbientBase(&pwm_result, &test_sensor, 1, 0);
        if(ret == 0)
        {
            printf("%d\t\t%d\n", test_temps[i], pwm_result);
        }
        else
        {
            printf("%d\t\tError: %d\n", test_temps[i], ret);
        }
    }
    printf("\n");
}

void test_polynomial_calibration()
{
    printf("=== Testing Polynomial Calibration ===\n");
    
    // Initialize polynomial calibration
    g_AmbientCalibration.CalType = FSC_AMBIENT_CAL_POLYNOMIAL;
    g_AmbientCalibration.CoeffCount = 4;
    
    // Example polynomial: ΔT = 8.0 - 0.05*PWM + 0.0001*PWM^2 - 0.000001*PWM^3
    g_AmbientCalibration.Coefficients[0] = 8.0;    // a0
    g_AmbientCalibration.Coefficients[1] = -0.05;  // a1
    g_AmbientCalibration.Coefficients[2] = 0.0001; // a2
    g_AmbientCalibration.Coefficients[3] = -0.000001; // a3
    
    printf("Polynomial Calibration Test (ΔT = 8.0 - 0.05*PWM + 0.0001*PWM^2 - 0.000001*PWM^3):\n");
    printf("PWM\tDelta_T\tInlet_Temp\tAmbient_Temp\n");
    
    INT16S inlet_temp = 45;  // Fixed inlet temperature
    INT8U test_pwm_values[] = {0, 25, 50, 75, 100};
    
    for(int i = 0; i < 5; i++)
    {
        float ambient_temp = FSCGetAmbientTemperature(inlet_temp, test_pwm_values[i], 1);
        float delta_t = inlet_temp - ambient_temp;
        printf("%d\t%.2f\t%d\t\t%.1f\n", 
               test_pwm_values[i], delta_t, inlet_temp, ambient_temp);
    }
    printf("\n");
}

int main()
{
    printf("Ambient Temperature Control Algorithm Test\n");
    printf("==========================================\n\n");
    
    test_ambient_calibration();
    test_polynomial_calibration();
    test_ambient_base_curve();
    
    printf("Test completed successfully!\n");
    return 0;
}
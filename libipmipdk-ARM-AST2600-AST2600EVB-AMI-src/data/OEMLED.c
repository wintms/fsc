/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc.                                          **
*** **
*** All Rights Reserved.                                                 **
*** **
*** **
*** File Name:       OEMLED.c                                            **
*** Description:     Implementation of platform-specific LED control and  **
*** status retrieval using CPLD/PLD registers.                           **
*** **
***************************************************************************
***************************************************************************
**************************************************************************/

#include <sys/time.h>
#include "Terminal.h"
#include "OEMLED.h"
#include "OEMDBG.h"
#include "OEMPLD.h"

/**
 * @brief Global flag indicating the current LED control mode (Auto or Manual).
 *
 * This variable is controlled by the OEM_SetGetLEDMode command.
 * - LED_Auto: BMC controls LEDs automatically (manual set is blocked).
 * - LED_Manu: Manual control of LEDs is allowed.
 */
INT8U g_LEDTestMode = 0;

/*-------------------------------------------------------------------------*
 * Function Implementations (Base Board CPLD)
 *-------------------------------------------------------------------------*/

/**
 * @fn GetSIDLedOnFrontPanel
 * @brief Reads the CPLD register to get the current state of the front panel SID LED.
 *
 * @param[out] pattern Pointer to store the current LED pattern (mapped from CPLD value).
 * @return 0 on success, -1 on failure.
 */
static int GetSIDLedOnFrontPanel(INT8U *pattern)
{
    INT8U led_reg_val = 0;
    int ret = 0;

    // Read the current LED control register value from the Base Board CPLD
    ret = OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_SID_LED_CTRL, 0, &led_reg_val, PLD_ReadRegister);
    if (ret != 0)
    {
        // CPLD read failed
        return -1;
    }

    // Map the raw CPLD register value to the standard LED pattern enumeration
    switch (led_reg_val)
    {
        case CPLD_SID_LED_OFF:
            *pattern = LED_OFF;
        break;

        case CPLD_SID_LED_BLUE_ON:
            *pattern = LED_SOLID_BLUE;
        break;

        case CPLD_SID_LED_BLUE_FLASHING:
            *pattern = LED_FLASHING_BLUE;
        break;

        case CPLD_SID_LED_AMBER_FLASHING:
            *pattern = LED_FLASHING_AMBER;
        break;

        default:
            // Unknown CPLD value
            return -1;
    }

    return 0;
}

/**
 * @fn SetSIDLedOnFrontPanel
 * @brief Writes to the CPLD register to set the state of the front panel SID LED.
 *
 * @param pattern The desired LED pattern (mapped to CPLD value).
 * @return 0 on success, -1 on failure.
 */
static int SetSIDLedOnFrontPanel(INT8U pattern)
{
    INT8U led_reg_val = 0;

    // Debug print the requested LED pattern
    if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
        printf("  > Front Panel SID LED pattern: ");

    // Map the standard LED pattern to the raw CPLD register value
    switch(pattern)
    {
        case LED_OFF:
            led_reg_val = CPLD_SID_LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("Off\n");
        break;

        case LED_SOLID_BLUE:
            led_reg_val = CPLD_SID_LED_BLUE_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("Solid Blue\n");
        break;

        // Flashing blue is typically used for the System ID / Beacon function
        case LED_FLASHING_BLUE:
            led_reg_val = CPLD_SID_LED_BLUE_FLASHING;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("Flashing Blue\n");
        break;

        case LED_FLASHING_AMBER:
            led_reg_val = CPLD_SID_LED_AMBER_FLASHING;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("Flashing Amber\n");
        break;

        default:
            // Unsupported pattern for this LED
            return -1;
    }

    // Write the calculated register value to the Base Board CPLD
    return OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_SID_LED_CTRL, led_reg_val, NULL, PLD_WriteRegister);
}

/**
 * @fn GetPowerLedOnFrontPanel
 * @brief Reads the CPLD register to get the current state of the front panel Power/PSU LED.
 *
 * @param[out] pattern Pointer to store the current LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int GetPowerLedOnFrontPanel(INT8U *pattern)
{
    INT8U led_reg_val = 0;
    int ret = 0;

    // Read the current LED control register value from the Base Board CPLD
    ret = OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_PSU_LED_CTRL, 0, &led_reg_val, PLD_ReadRegister);
    if (ret != 0)
    {
        return -1;
    }

    // Map the raw CPLD register value to the standard LED pattern enumeration
    switch(led_reg_val)
    {
        case CPLD_PSU_LED_OFF:
            *pattern = LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case CPLD_PSU_LED_GREEN_ON:
            *pattern = LED_SOLID_GREEN;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case CPLD_PSU_LED_AMBER_FLASHING:
            *pattern = LED_FLASHING_AMBER;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        case CPLD_PSU_LED_AMBER_ON:
            *pattern = LED_SOLID_AMBER;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Amber\n");
        break;

        default:
            // Unknown CPLD value
            return -1;
    }

    return 0;
}

/**
 * @fn SetPowerLedOnFrontPanel
 * @brief Writes to the CPLD register to set the state of the front panel Power/PSU LED.
 *
 * @param pattern The desired LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int SetPowerLedOnFrontPanel(INT8U pattern)
{
    INT8U led_reg_val = 0;

    // Debug print the requested LED pattern
    if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
        printf("  >> Front Panel Power LED pattern: ");

    // Map the standard LED pattern to the raw CPLD register value
    switch(pattern)
    {
        case LED_OFF:
            led_reg_val = CPLD_PSU_LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case LED_SOLID_GREEN:
            led_reg_val = CPLD_PSU_LED_GREEN_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case LED_FLASHING_AMBER:
            led_reg_val = CPLD_PSU_LED_AMBER_FLASHING;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        case LED_SOLID_AMBER:
            led_reg_val = CPLD_PSU_LED_AMBER_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Amber\n");
        break;

        default:
            // Unsupported pattern for this LED
            return -1;
    }

    // Write the calculated register value to the Base Board CPLD
    return OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_PSU_LED_CTRL, led_reg_val, NULL, PLD_WriteRegister);
}

/**
 * @fn GetFanLedOnFrontPanel
 * @brief Reads the CPLD register to get the current state of the front panel Fan LED.
 *
 * @param[out] pattern Pointer to store the current LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int GetFanLedOnFrontPanel(INT8U *pattern)
{
    INT8U led_reg_val = 0;
    int ret = 0;

    // Read the current LED control register value from the Base Board CPLD
    ret = OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_FAN_LED_CTRL, 0, &led_reg_val, PLD_ReadRegister);
    if (ret != 0)
    {
        return -1;
    }
    
    // Map the raw CPLD register value to the standard LED pattern enumeration
    switch(led_reg_val)
    {
        case CPLD_FAN_LED_OFF:
            *pattern = LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case CPLD_FAN_LED_GREEN_ON:
            *pattern = LED_SOLID_GREEN;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case CPLD_FAN_LED_AMBER_FLASHING:
            *pattern = LED_FLASHING_AMBER;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        case CPLD_FAN_LED_AMBER_ON:
            *pattern = LED_SOLID_AMBER;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Amber\n");
        break;

        default:
        	// Unknown CPLD value
            return -1;
    }

    return 0;
}

/**
 * @fn SetFanLedOnFrontPanel
 * @brief Writes to the CPLD register to set the state of the front panel Fan LED.
 *
 * @param pattern The desired LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int SetFanLedOnFrontPanel(INT8U pattern)
{
    INT8U led_reg_val = 0;

    // Debug print the requested LED pattern
    if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
        printf("  >> Front Panel FAN LED pattern: ");

    // Map the standard LED pattern to the raw CPLD register value
    switch(pattern)
    {
        case LED_OFF:
            led_reg_val = CPLD_FAN_LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case LED_SOLID_GREEN:
            led_reg_val = CPLD_FAN_LED_GREEN_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case LED_FLASHING_AMBER:
            led_reg_val = CPLD_FAN_LED_AMBER_FLASHING;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        case LED_SOLID_AMBER:
            led_reg_val = CPLD_FAN_LED_AMBER_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Amber\n");
        break;

        default:
            // Unsupported pattern for this LED
            return -1;
    }

    // Write the calculated register value to the Base Board CPLD
    OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_FAN_LED_CTRL, led_reg_val, NULL, PLD_WriteRegister);

    return 0;
}

/*-------------------------------------------------------------------------*
 * Static Function Implementations (Fan Board CPLD)
 *-------------------------------------------------------------------------*/

/**
 * @fn GetFanLedOnFanCage
 * @brief Reads the CPLD register to get the current state of a specific fan tray LED.
 *
 * This function handles individual fan LEDs which are typically controlled by a Fan Board CPLD.
 *
 * @param led The LED number corresponding to a fan tray (FAN1_LED to FAN5_LED).
 * @param[out] pattern Pointer to store the current LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int GetFanLedOnFanCage(INT8U led, INT8U *pattern)
{
    INT8U led_reg;
    INT8U led_reg_val = 0;
    int ret = 0;

    // Determine the CPLD register address based on the fan LED number
    switch(led)
    {
        case FAN1_LED:
            led_reg = CPLD_F_FAN1_LED_CTRL;
        break;

        case FAN2_LED:
            led_reg = CPLD_F_FAN2_LED_CTRL;
        break;

        case FAN3_LED:
            led_reg = CPLD_F_FAN3_LED_CTRL;
        break;

        case FAN4_LED:
            led_reg = CPLD_F_FAN4_LED_CTRL;
        break;

        case FAN5_LED:
            led_reg = CPLD_F_FAN5_LED_CTRL;
        break;

        default:
            return -1;
    }

    // Read the current LED control register value from the Fan Board CPLD
    ret = OEM_ReadWritePLD(PLD_ID_FanBoard, led_reg, 0, &led_reg_val, PLD_ReadRegister);
    if (ret != 0)
    {
        return -1;
    }

    // Extract only the LED pattern bits
    led_reg_val &= CPLD_FAN_TRAY_LED_BITS_MASK;

    // Map the raw CPLD register value to the standard LED pattern enumeration
    switch(led_reg_val)
    {
        case CPLD_FAN_TRAY_LED_OFF:
            *pattern = LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case CPLD_FAN_TRAY_LED_GREEN_ON:
            *pattern = LED_SOLID_GREEN;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case CPLD_FAN_TRAY_LED_AMBER_FLASHING:
            *pattern = LED_FLASHING_AMBER;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        default:
        	// Unknown CPLD value
            return -1;
    }

    return 0;
}

/**
 * @fn SetFanLedOnFanCage
 * @brief Writes to the CPLD register to set the state of a specific fan tray LED.
 *
 * The CPLD register value is constructed by ORing the software control bit and the pattern value.
 *
 * @param led The LED number corresponding to a fan tray (FAN1_LED to FAN5_LED).
 * @param pattern The desired LED pattern.
 * @return 0 on success, -1 on failure.
 */
static int SetFanLedOnFanCage(INT8U led, INT8U pattern)
{
    INT8U led_reg;
    INT8U fan_id;
    INT8U led_reg_val = CPLD_FAN_TRAY_LED_SOFT_CTRL;

    // Determine the CPLD register address and fan ID based on the LED number
    switch(led)
    {
        case FAN1_LED:
            led_reg = CPLD_F_FAN1_LED_CTRL;
            fan_id = 1;
        break;

        case FAN2_LED:
            led_reg = CPLD_F_FAN2_LED_CTRL;
            fan_id = 2;
        break;

        case FAN3_LED:
            led_reg = CPLD_F_FAN3_LED_CTRL;
            fan_id = 3;
        break;

        case FAN4_LED:
            led_reg = CPLD_F_FAN4_LED_CTRL;
            fan_id = 4;
        break;

        case FAN5_LED:
            led_reg = CPLD_F_FAN5_LED_CTRL;
            fan_id = 5;
        break;

        default:
            return -1;
    }

    // Debug print the requested LED pattern
    if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
        printf("  >> Fan tray Fan%d LED pattern: ", fan_id);

    // Map the standard LED pattern to the raw CPLD value and OR it with the control bits
    switch(pattern)
    {
        case LED_OFF:
            led_reg_val |= CPLD_FAN_TRAY_LED_OFF;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Off\n");
        break;

        case LED_SOLID_GREEN:
            led_reg_val |= CPLD_FAN_TRAY_LED_GREEN_ON;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Solid Green\n");
        break;

        case LED_FLASHING_AMBER:
            led_reg_val |= CPLD_FAN_TRAY_LED_AMBER_FLASHING;
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0) printf("Flashing Amber\n");
        break;

        default:
            // Unsupported pattern for this LED
            return -1;
    }

    // Write the calculated register value to the Fan Board CPLD
    return OEM_ReadWritePLD(PLD_ID_FanBoard, led_reg, led_reg_val, NULL, PLD_WriteRegister);
}

/*-------------------------------------------------------------------------*
 * External Function Implementations (API)
 *-------------------------------------------------------------------------*/

/**
 * @fn GetLEDPatterns
 * @brief Gets the current pattern of a specified LED.
 *
 * This function acts as a dispatcher, calling the correct platform-specific
 * helper function based on the provided LED identifier.
 *
 * @param LEDNum The identifier for the LED.
 * @param[out] Pattern Pointer to store the current LED pattern.
 * @return 0 on success, -1 on failure.
 */
int GetLEDPatterns(INT8U LEDNum, INT8U *Pattern)
{
    int ret = 0;

    switch(LEDNum)
    {
        case FrontPanel_POWER_LED:
            ret = GetPowerLedOnFrontPanel(Pattern);
            break;

        case FrontPanel_FAN_LED:
            ret = GetFanLedOnFrontPanel(Pattern);
            break;

        case FrontPanel_SID_LED:
            ret = GetSIDLedOnFrontPanel(Pattern);
            break;

        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
        case FAN5_LED:
            // Handle Fan Cage LEDs
            ret = GetFanLedOnFanCage(LEDNum, Pattern);
            break;

        default:
            // Invalid LED ID provided
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("  >> Invalid LED number\n");

            return -1;
    }

    return ret;
}

/**
 * @fn SetLEDPatterns
 * @brief Sets the pattern for a specified LED.
 *
 * This function acts as a dispatcher, calling the correct platform-specific
 * helper function based on the provided LED identifier.
 *
 * @param LEDNum The identifier for the LED.
 * @param Pattern The desired pattern to set.
 * @return 0 on success, -1 on failure, -2 if operation not supported in the current state.
 */
int SetLEDPatterns(INT8U LEDNum, INT8U Pattern)
{
    int ret = 0;

    // Check the global LED control mode. If in automatic (LED_Auto) mode,
    // prevent manual pattern setting and return CC_PARAM_NOT_SUP_IN_CUR_STATE (-2).
    if (g_LEDTestMode == LED_Auto)
    {
        printf("  >> set LED pattern not supported in the current state.\n");
        return -2;
    }

    if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
        printf("  > SetLEDPatterns for LED: 0x%02X, Pattern: 0x%02X\n", LEDNum, Pattern);

    switch(LEDNum)
    {
        case FrontPanel_POWER_LED:
            ret = SetPowerLedOnFrontPanel(Pattern);
            break;

        case FrontPanel_FAN_LED:
            ret = SetFanLedOnFrontPanel(Pattern);
            break;

        case FrontPanel_SID_LED:
            ret = SetSIDLedOnFrontPanel(Pattern);
            break;

        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
        case FAN5_LED:
            // Handle Fan Cage LEDs
            ret = SetFanLedOnFanCage(LEDNum, Pattern);
            break;

        default:
            // Invalid LED ID provided
            if(g_OEMDebugArray[OEM_DEBUG_Item_LED] > 0)
                printf("  >> Invalid LED number\n");

            return -1;
    }

    return ret;
}


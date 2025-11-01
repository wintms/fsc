/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc. **
*** **
*** All Rights Reserved. **
*** **
*** **
*** File Name: OEMFAN.c **
*** Description: Source file for OEM fan control and monitoring functions. **
*** **
***************************************************************************
***************************************************************************
**************************************************************************/

#include "hal_hw.h"
#include "NVRAccess.h"
#include "PDKEEPROM.h"

#include "OEMFAN.h"
#include "OEMPLD.h"
#include "OEMDBG.h"
#include "OEMSysInfo.h"
#include "OEMFRU.h"
#include "libtlv.h"

/**
 * @fn OEM_GetSystemAirflow
 * @brief Gets the cached overall system airflow direction.
 * @return The system airflow direction (e.g., AIRFLOW_F2B, AIRFLOW_B2F, or AIRFLOW_UNKNOWN).
 */
int OEM_GetSystemAirflow(void)
{
    return g_SystemInfo.systemAirflow;
}

/**
 * @fn OEM_GetFanTrayPresent
 * @brief Get fan present status from CPLD register.
 * @param fan_id The fan tray ID (SYS_FAN1, SYS_FAN2, etc.).
 * @return
 * - FAN_PRESENT (1) if the fan is present.
 * - FAN_ABSENT (0) if the fan is absent.
 * - -1 on failure to read from CPLD.
 */
int OEM_GetFanTrayPresent(INT8U fan_id)
{
    INT8U present_reg = 0;
    INT8U status = 0;
    INT8U presence_bit = 0;
    int ret = 0;

    switch(fan_id)
    {
        case SYS_FAN1:
            present_reg = CPLD_F_FAN1_PRESENT_STAT;
            break;

        case SYS_FAN2:
            present_reg = CPLD_F_FAN2_PRESENT_STAT;
            break;

        case SYS_FAN3:
            present_reg = CPLD_F_FAN3_PRESENT_STAT;
            break;

        case SYS_FAN4:
            present_reg = CPLD_F_FAN4_PRESENT_STAT;
            break;

        case SYS_FAN5:
            present_reg = CPLD_F_FAN5_PRESENT_STAT;
            break;

        default:
            return -1;
            break;
    }

    // Read the entire status register (all 8 bits) from the CPLD.
    ret = OEM_ReadWritePLD(PLD_ID_FanBoard, present_reg, 0, &status, PLD_ReadRegister);
    if (ret != 0)
    {
        if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
        {
            printf("  >> Fan%d present status read failed\n", fan_id + 1);
        }
        return -1;
    }

    // Isolate the presence bit (LSB) by masking with 0x01.
    // As per the HW design: Bit 0 = 0 means PRESENT, Bit 0 = 1 means ABSENT.
    presence_bit = status & CPLD_FAN_TRAY_PRESENT_MASK;

    if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
    {
        printf("  >> Fan%d present: %s\n", fan_id + 1, (CPLD_FAN_TRAY_PRESENT == presence_bit) ? "Yes" : "No");
    }

    if(CPLD_FAN_TRAY_PRESENT == presence_bit)
        return FAN_PRESENT;

    return FAN_ABSENT;
}

/**
 * @fn OEM_GetFanTrayRPM
 * @brief Get the RPM for a specific fan rotor.
 * @param fan_id The fan tray ID (SYS_FAN1, SYS_FAN2, etc.).
 * @param fan_rotor The rotor to read (FAN_ROTOR_FRONT or FAN_ROTOR_REAR).
 * @param[out] rpm Pointer to store the fan speed in RPM.
 * @return 0 on success, -1 on failure.
 */
int OEM_GetFanTrayRPM(INT8U fan_id, INT8U fan_rotor, INT16U *rpm)
{
    INT8U rpm_reg = 0, rpm_reg_front = 0, rpm_reg_rear = 0;
    INT8U rpm_raw = 0;
    int ret = 0;

    if (FAN_ABSENT == OEM_GetFanTrayPresent(fan_id))
    {
        return -1;
    }

    switch (fan_id)
    {
        case SYS_FAN1:
            rpm_reg_front = CPLD_F_FAN1_FRONT_RPM;
            rpm_reg_rear = CPLD_F_FAN1_REAR_RPM;
            break;
        case SYS_FAN2:
            rpm_reg_front = CPLD_F_FAN2_FRONT_RPM;
            rpm_reg_rear = CPLD_F_FAN2_REAR_RPM;
            break;
        case SYS_FAN3:
            rpm_reg_front = CPLD_F_FAN3_FRONT_RPM;
            rpm_reg_rear = CPLD_F_FAN3_REAR_RPM;
            break;
        case SYS_FAN4:
            rpm_reg_front = CPLD_F_FAN4_FRONT_RPM;
            rpm_reg_rear = CPLD_F_FAN4_REAR_RPM;
            break;
        case SYS_FAN5:
            rpm_reg_front = CPLD_F_FAN5_FRONT_RPM;
            rpm_reg_rear = CPLD_F_FAN5_REAR_RPM;
            break;
        default:
            return -1;
    }

    if (FAN_ROTOR_FRONT == fan_rotor)
    {
        rpm_reg = rpm_reg_front;
    }
    else
    {
        rpm_reg = rpm_reg_rear;
    }

    ret = OEM_ReadWritePLD(PLD_ID_FanBoard, rpm_reg, 0, &rpm_raw, PLD_ReadRegister);
    if (ret != 0)
    {
        if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
        {
            printf("  >> Fan%d rpm read failed\n", fan_id + 1);
        }
        return -1;
    }

    *rpm = (INT16U)(rpm_raw * FAN_RPM_MULTIPLIER);

    if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
    {
        printf("  >> Fan%d-%s RPM: %d\n", fan_id + 1, (FAN_ROTOR_FRONT == fan_rotor) ? "Front" : "Rear", *rpm);
    }

    return 0;
}

/**
 * @fn OEM_SetFanTrayPWM
 * @brief Set the PWM duty cycle for a specific fan tray.
 * @param fan_id The fan tray ID (SYS_FAN1, SYS_FAN2, etc.).
 * @param pwm_duty_cycle The PWM duty cycle percentage to set (0-100).
 * @return 0 on success, -1 on failure.
 */
int OEM_SetFanTrayPWM(INT8U fan_id, INT8U pwm_duty_cycle)
{
    INT8U pwm_reg = 0;
    INT8U cpld_pwm_val = 0;
    int ret = 0;

    if (FAN_ABSENT == OEM_GetFanTrayPresent(fan_id))
    {
        return -1;
    }

    switch (fan_id)
    {
        case SYS_FAN1:
            pwm_reg = CPLD_F_FAN1_PWM_CTRL;
            break;
        case SYS_FAN2:
            pwm_reg = CPLD_F_FAN2_PWM_CTRL;
            break;
        case SYS_FAN3:
            pwm_reg = CPLD_F_FAN3_PWM_CTRL;
            break;
        case SYS_FAN4:
            pwm_reg = CPLD_F_FAN4_PWM_CTRL;
            break;
        case SYS_FAN5:
            pwm_reg = CPLD_F_FAN5_PWM_CTRL;
            break;
        default:
            return -1;
    }

    // Validate the input duty cycle is within the 0-100% range.
    if (pwm_duty_cycle > 100)
    {
        pwm_duty_cycle = 100;
    }

    // Convert percentage (0-100) to CPLD value (0-255)
    cpld_pwm_val = (INT8U)((pwm_duty_cycle * 255) / 100);

    if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
    {
        printf("  >> Setting Fan%d PWM to: %d\n", fan_id + 1, pwm_duty_cycle);
    }

    ret = OEM_ReadWritePLD(PLD_ID_FanBoard, pwm_reg, cpld_pwm_val, NULL, PLD_WriteRegister);
    return ret;
}

/**
 * @fn OEM_SetAllFanTraysPWM
 * @brief Set the PWM duty cycle for all fan trays.
 * @param pwm_duty_cycle The PWM duty cycle percentage to set (0-100).
 * @return 0 on success, -1 on failure.
 */
int OEM_SetAllFanTraysPWM(INT8U pwm_duty_cycle)
{
    int i;
    int final_ret = 0;

    for (i = 0; i < SYS_FAN_NUM_MAX; i++)
    {
        // OEM_SetFanTrayPWM returns -1 if the fan is absent or if the write fails.
        if (OEM_SetFanTrayPWM(i, pwm_duty_cycle) != 0)
        {
            final_ret = -1; // Indicate that at least one fan PWM set failed.
        }
    }
    return final_ret;
}

/**
 * @fn OEM_GetFanTrayAirflow
 * @brief Get Fan ariflow status.
 * @param fan_id The fan tray ID (SYS_FAN1, SYS_FAN2, etc.).
 * @return AIRFLOW_F2B or AIRFLOW_B2F, -1 if other failures.
 */
int OEM_GetFanTrayAirflow(INT8U fan_id)
{
    INT8U bus;
    INT8U addr;
    INT32U iana;
    INT8U actual_len = 0;
    int ret;
    FanTLVDellExtData_T tlv_data;
    INT8U airflow = AIRFLOW_UNKNOWN;

    if(FAN_ABSENT == OEM_GetFanTrayPresent(fan_id))
    {
        return -1;
    }

    switch(fan_id)
    {
        case SYS_FAN1:
            bus = FRU_EEPROM_BUS_FAN1;
            addr = FRU_EEPROM_ADDR_FAN1;
            break;

        case SYS_FAN2:
            bus = FRU_EEPROM_BUS_FAN2;
            addr = FRU_EEPROM_ADDR_FAN2;
            break;

        case SYS_FAN3:
            bus = FRU_EEPROM_BUS_FAN3;
            addr = FRU_EEPROM_ADDR_FAN3;
            break;

        case SYS_FAN4:
            bus = FRU_EEPROM_BUS_FAN4;
            addr = FRU_EEPROM_ADDR_FAN4;
            break;

        case SYS_FAN5:
            bus = FRU_EEPROM_BUS_FAN5;
            addr = FRU_EEPROM_ADDR_FAN5;
            break;

        default:
            if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
            {
                printf("  >> Fan id %d not supported\n", fan_id);
            }
            return -1;
    }

    ret = tlv_eeprom_read(bus, (addr >> 1), EEPROM_24C64_SIZE,
        TLV_CODE_VENDOR_EXT, (INT8U *)&tlv_data, sizeof(FanTLVDellExtData_T), &actual_len);
    if (TLV_SUCCESS != ret)
    {
        if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
        {
            printf("  >> Fan%d TLV EEPROM read failed, ret = %d\n", fan_id + 1, ret);
        }
        return -1;
    }

    // Ensure the IANA number matches Dell's specific IANA to confirm it's a Dell fan.
    iana = (tlv_data.IANANo[0] << 24) | (tlv_data.IANANo[1] << 16) | (tlv_data.IANANo[2] << 8) | tlv_data.IANANo[3];
    if (iana != FAN_EEPROM_DELL_IANA)
    {
        if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
        {
            printf("  >> Fan%d IANA code mismatch, IANA = 0x%x\n", fan_id + 1, iana);
        }
        return -1;
    }

    if (tlv_data.Type == FAN_EEPROM_DELL_F2B)
    {
        airflow = AIRFLOW_F2B;
    }
    else if (tlv_data.Type == FAN_EEPROM_DELL_B2F)
    {
        airflow = AIRFLOW_B2F;
    }

    if(g_OEMDebugArray[OEM_DEBUG_Item_FAN] > 0)
    {
        if (AIRFLOW_B2F == airflow || AIRFLOW_F2B == airflow)
        {
            printf("  >> Fan%d airflow: %s\n", fan_id + 1, (AIRFLOW_F2B == airflow) ? "F2B" : "B2F" );
        }
        else
        {
            printf("  >> Fan%d airflow: %s\n", fan_id + 1, "Unknown" );
        }
    }

    return airflow;
}

#include "OEMFAN.h"

/**
 * @brief Global variable tracking the Fan Speed Control (FSC) mode.
 *
 * Mode is typically 0 (FAN_CTL_MODE_AUTO) or 1 (FAN_CTL_MODE_MANUAL).
 */
static INT8U g_FanControlMode = FAN_CTL_MODE_AUTO;

void SetFanControlMode(INT8U mode)
{
    g_FanControlMode = mode;
}

INT8U GetFanControlMode(void)
{
    return g_FanControlMode;
}
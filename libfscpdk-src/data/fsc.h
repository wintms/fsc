#ifndef FSC_H
#define FSC_H

#include "Types.h"

#define     FAN_CTL_MODE_AUTO           1
#define     FAN_CTL_MODE_MANUAL         0
#define     FAN_CTL_MODE_MANUAL_FILE    "/var/fsc_manual"

extern int SetFanControlMode( INT8U Statues, int BMCInst );
extern int GetFanControlMode( INT8U *Status, int BMCInst );
extern int FanControlLoop(int BMCInst);

#endif // FSC_H
#ifndef FSC_H
#define FSC_H

#include "Types.h"

extern int FanControlLoop(int BMCInst);
extern void SetFanControlMode(INT8U mode);
extern INT8U GetFanControlMode(void);

#endif // FSC_H
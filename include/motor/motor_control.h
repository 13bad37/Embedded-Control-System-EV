#pragma once

#include <stdint.h>
#include "common/project_types.h"

void MotorControl_Init(void);
void MotorControl_Run(void);
void MotorControl_Start(void);
void MotorControl_Stop(void);
void MotorControl_SetSpeed(uint32_t rpm);
void MotorControl_EStop(EvFaultCode_t fault);
uint32_t MotorControl_GetSpeed(void);
EvMotorState_t MotorControl_GetState(void);
EvMotorStatus_t MotorControl_GetStatus(void);

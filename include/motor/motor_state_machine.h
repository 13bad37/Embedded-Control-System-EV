#pragma once

#include <stdbool.h>
#include "common/project_types.h"

void MotorStateMachine_Init(void);
void MotorStateMachine_Run(void);
void MotorStateMachine_RequestStart(void);
void MotorStateMachine_RequestStop(void);
void MotorStateMachine_RequestEStop(EvFaultCode_t fault);
void MotorStateMachine_AcknowledgeFault(void);
void MotorStateMachine_SetHallValid(bool valid);
void MotorStateMachine_SetSafetyInterlock(bool active, EvFaultCode_t fault);
void MotorStateMachine_UpdateActualRpm(uint32_t rpm);
EvMotorState_t MotorStateMachine_GetState(void);
EvFaultCode_t  MotorStateMachine_GetLatchedFault(void);

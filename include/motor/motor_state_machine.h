#pragma once

#include "common/project_types.h"

void MotorStateMachine_Init(void);
void MotorStateMachine_Run(void);
void MotorStateMachine_RequestStart(void);
void MotorStateMachine_RequestStop(void);
void MotorStateMachine_RequestEStop(EvFaultCode_t fault);
void MotorStateMachine_AcknowledgeFault(void);
EvMotorState_t MotorStateMachine_GetState(void);

#pragma once

#include "common/project_types.h"

void FaultManager_Init(void);
void FaultManager_Run(void);
void FaultManager_RecordFault(EvFaultCode_t fault);


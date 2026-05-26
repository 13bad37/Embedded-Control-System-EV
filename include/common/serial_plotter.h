#pragma once

#include "common/project_types.h"
#include "common/sensor_types.h"

void SerialPlotter_Init(void);
void SerialPlotter_SendFrame(const EvMotorStatus_t *status,
                             const EvSensorDataRaw_t *raw,
                             const EvSensorData_t *filtered);

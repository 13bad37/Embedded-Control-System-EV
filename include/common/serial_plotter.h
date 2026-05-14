#pragma once

#include "common/project_types.h"
#include "common/sensor_types.h"
#include "sensors/sensor_manager.h"

void SerialPlotter_Init(void);
void SerialPlotter_SendMotorFrame(const EvMotorStatus_t *status);
void SerialPlotter_SendSensorFrame(const EvSensorDataRaw_t *raw, const EvSensorData_t *filtered);


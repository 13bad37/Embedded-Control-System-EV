#pragma once

#include "common/project_types.h"
#include "common/sensor_types.h"

void SerialPlotter_Init(void);
void SerialPlotter_SendMotorFrame(const EvMotorStatus_t *status);
void SerialPlotter_SendSensorFrame(const EvSensorData_t *raw, const EvSensorData_t *filtered);


#pragma once

#include "common/sensor_types.h"

void SensorFilter_Init(void);
void SensorFilter_Update(const EvSensorData_t *raw, EvSensorData_t *filtered);


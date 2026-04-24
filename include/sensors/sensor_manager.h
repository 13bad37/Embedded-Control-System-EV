#pragma once

#include "common/sensor_types.h"

void SensorManager_Init(void);
void SensorManager_Poll(void);
EvSensorData_t SensorManager_GetRawData(void);
EvSensorData_t SensorManager_GetFilteredData(void);
EvSensorThresholds_t SensorManager_GetThresholds(void);
void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds);

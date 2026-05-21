#pragma once

#include "common/sensor_types.h"

void SensorManager_Init(void);
void SensorManager_Poll(void);
typedef struct
{
    int32_t lux;
    int32_t temperatureC;
    int32_t humidityRh;
    uint16_t distanceMm;
    uint32_t motor_speed_rpm;
    uint32_t motor_power_w;
} EvSensorDataRaw_t;



EvSensorDataRaw_t SensorManager_GetRawData(void);
EvSensorData_t SensorManager_GetFilteredData(void);
EvSensorThresholds_t SensorManager_CheckThresholds(void);
void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds);

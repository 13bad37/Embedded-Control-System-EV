#pragma once

#include "common/sensor_types.h"

void SensorManager_Init(void);
void SensorManager_Poll(void);
EvSensorData_t SensorManager_GetRawData(void);
EvSensorData_t SensorManager_GetFilteredData(void);
EvSensorThresholds_t SensorManager_GetThresholds(void);
void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds);


typedef enum {
    MSG_LUX,
    MSG_SHT31,
    MSG_VL53L0X
} SensorMsgType_t;

typedef struct {
    SensorMsgType_t type;
    union {
        int lux;
        struct { int tempC; int humidPct; } sht31;
        uint16_t distance_mm;
    } data;
} SensorMsg_t;
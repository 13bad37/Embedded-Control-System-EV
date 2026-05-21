#pragma once

#include "common/sensor_types.h"
#include "queue.h"


void SensorManager_Init(void);
void SensorManager_Poll(void);
typedef struct
{
    uint32_t lux;
    uint32_t temperatureC;
    uint32_t humidityRh;
    uint32_t distanceMm;
    uint32_t motor_speed_rpm;
    uint32_t motor_power_w;
} EvSensorDataRaw_t;

typedef enum {
    SENSOR_MSG_SPEED,
    SENSOR_MSG_LIGHT,
    SENSOR_MSG_TEMP,
    SENSOR_MSG_HUMID,
    SENSOR_MSG_DISTANCE,
    SENSOR_MSG_POWER,
} SensorMsgType_t;

typedef struct {
    SensorMsgType_t type;
    uint32_t value;
} SensorMsg_t;
extern QueueHandle_t rawdataQueue;

EvSensorDataRaw_t SensorManager_GetRawData(void);
EvSensorData_t SensorManager_GetFilteredData(void);
EvSensorThresholds_t SensorManager_CheckThresholds(void);
void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds);

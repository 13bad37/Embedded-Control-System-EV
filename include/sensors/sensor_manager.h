#pragma once

#include "common/sensor_types.h"
#include "queue.h"


void SensorManager_Init(void);
void SensorManager_Poll(void);


typedef enum {
    SENSOR_MSG_SPEED,
    SENSOR_MSG_LIGHT,
    SENSOR_MSG_TEMP,
    SENSOR_MSG_HUMID,
    SENSOR_MSG_DISTANCE,
    SENSOR_MSG_POWERA,
    SENSOR_MSG_POWERB
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

#pragma once

#include "common/sensor_types.h"

typedef struct {
    int32_t  buffer[8]; // Storage window
    uint32_t index;                          // Points to current write location
    uint32_t count;                          // Tracks how many valid entries exist
} MovingAverageFilter_t;

void SensorFilter_Init(void);
void SensorFilter_Update(const EvSensorData_t *raw, EvSensorData_t *filtered);

int32_t Filter_ApplyMovingAverage(MovingAverageFilter_t *filter, int32_t newSample, uint8_t size);

uint16_t Vl53l0xSensor_UpdateFilter(uint16_t rawDistanceMm);

extern MovingAverageFilter_t g_lightFilter;
extern MovingAverageFilter_t g_tempFilter;
extern MovingAverageFilter_t g_humidFilter;
extern MovingAverageFilter_t g_speedFilter;
extern MovingAverageFilter_t g_powerFilter;
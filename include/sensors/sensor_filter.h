#pragma once

#include "common/sensor_types.h"

#include <stdint.h>

#define FILTER_WINDOW_SIZE 8

typedef struct {
    int lux;
    int tempC;
    int humidPct;
    int distance_mm;
} SensorFilteredData_t;

void SensorFilter_Init(void);
void SensorFilter_Update(int lux, int tempC, int humidPct, int distance_mm, SensorFilteredData_t *pOut);
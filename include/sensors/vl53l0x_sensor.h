#ifndef VL53L0X_SENSOR_H
#define VL53L0X_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

void Vl53l0xSensor_Init(void);

void Vl53l0xSensor_Sample(void);

bool Vl53l0xSensor_GetRawDistanceMm(uint16_t *distanceMm);

bool Vl53l0xSensor_GetFilteredDistanceMm(uint16_t *distanceMm);

#endif
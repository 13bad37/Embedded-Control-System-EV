#ifndef SHT31_SENSOR_H
#define SHT31_SENSOR_H

#include <stdint.h>
#include <stdbool.h>


void Sht31Sensor_Init(void);

void Sht31Sensor_Sample(void);

bool Sht31Sensor_GetRawTemperatureC(int32_t *tempC);

bool Sht31Sensor_GetRawHumidityPct(int32_t *humidPct);

bool Sht31Sensor_GetFilteredTemperatureC(int32_t *tempC);

bool Sht31Sensor_GetFilteredHumidityPct(int32_t *humidPct);

#endif
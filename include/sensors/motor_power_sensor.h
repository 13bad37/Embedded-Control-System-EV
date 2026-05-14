#ifndef MOTOR_POWER_SENSOR_H
#define MOTOR_POWER_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#define POWER_VREF_V                3.3f
#define POWER_ADC_RESOLUTION        4095.0f
#define POWER_SHUNT_OHM             0.007f
#define POWER_CSA_GAIN              10.0f
#define POWER_SUPPLY_V              24.0f


#define POWER_SAMPLE_RATE_HZ        150

// Moving average filter window — larger = 100ms per average for 150Hz
#define POWER_FILTER_WINDOW         15

#define POWER_ESTOP_THRESHOLD_W     500.0f

void MotorPowerSensor_Init(void);

float MotorPowerSensor_GetFilteredPowerW(void);

float MotorPowerSensor_GetRawPowerW(void);

bool MotorPowerSensor_IsOvercurrent(void);

void MotorPower_ADC_ISR(void);

#endif // MOTOR_POWER_SENSOR_H
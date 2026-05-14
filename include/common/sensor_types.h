#pragma once

#include <stdint.h>

typedef enum
{
    EV_SENSOR_MOTOR_POWER = 0,
    EV_SENSOR_MOTOR_SPEED_HALL,
    EV_SENSOR_OPT3001_AMBIENT_LIGHT,
    EV_SENSOR_SHT31_TEMP_HUMIDITY,
    EV_SENSOR_VL53L0X_DISTANCE,
    EV_SENSOR_COUNT
} EvSensorId_t;

typedef struct
{
    uint32_t motor_power_w;
    uint32_t motor_speed_rpm;
    uint32_t ambient_light_lux;
    int16_t cabin_temperature_c_x10;
    uint16_t cabin_humidity_rh_x10;
    uint16_t distance_mm;
} EvSensorData_t;

typedef struct
{
    uint32_t max_motor_power_w;
    uint16_t min_distance_mm;
} EvSensorThresholds_t;

#include "sensors/sensor_manager.h"

#include "sensors/ambient_light_sensor.h"
#include "sensors/hall_speed_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"

static EvSensorData_t s_rawData;
static EvSensorData_t s_filteredData;
static EvSensorThresholds_t s_thresholds;

void SensorManager_Init(void)
{
    SensorFilter_Init();
    MotorPowerSensor_Init();
    HallSpeedSensor_Init();
    AmbientLightSensor_Init();
    Sht31Sensor_Init();
    Vl53l0xSensor_Init();
}

void SensorManager_Poll(void)
{
    /*
     * Write up properly once sensor drivers are ready
     * For now this just gives the RTOS task a nice integration point.
     */
    MotorPowerSensor_Sample();
    HallSpeedSensor_Sample();
    AmbientLightSensor_Sample();
    Sht31Sensor_Sample();
    Vl53l0xSensor_Sample();
    SensorFilter_Update(&s_rawData, &s_filteredData);
}

EvSensorData_t SensorManager_GetRawData(void)
{
    return s_rawData;
}

EvSensorData_t SensorManager_GetFilteredData(void)
{
    return s_filteredData;
}

EvSensorThresholds_t SensorManager_GetThresholds(void)
{
    return s_thresholds;
}

void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds)
{
    if (thresholds != 0)
    {
        s_thresholds = *thresholds;
    }
}

#include "sensors/sensor_filter.h"

void SensorFilter_Init(void)
{

}

void SensorFilter_Update(const EvSensorData_t *raw, EvSensorData_t *filtered)
{
    (void)raw;
    (void)filtered;
    /* TODO: apply per sensor filters in RTOS task. */
}


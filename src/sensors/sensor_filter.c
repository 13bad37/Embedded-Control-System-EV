#include "sensors/sensor_filter.h"

#define VL53L0X_FILTER_SIZE             5

static int s_filterIndex = 0;
static int s_filterCount = 0;



static uint16_t s_filterBuffer[VL53L0X_FILTER_SIZE];
void SensorFilter_Init(void)
{

}

void SensorFilter_Update(const EvSensorData_t *raw, EvSensorData_t *filtered)
{
    (void)raw;
    (void)filtered;
    /* TODO: apply per sensor filters in RTOS task. */
}


int32_t Filter_ApplyMovingAverage(MovingAverageFilter_t *filter, int32_t newSample)
{
    filter->buffer[filter->index] = newSample;
    filter->index = (filter->index + 1) % 8;

    if (filter->count < 8)
    {
        filter->count++;
    }

    int64_t sum = 0;
    for (uint32_t i = 0; i < filter->count; i++)
    {
        sum += filter->buffer[i];
    }

    return (int32_t)(sum / (int64_t)filter->count);
}

uint16_t Vl53l0xSensor_Median3(uint16_t a, uint16_t b, uint16_t c)
{
    uint16_t temp;

    if (a > b)
    {
        temp = a;
        a = b;
        b = temp;
    }

    if (b > c)
    {
        temp = b;
        b = c;
        c = temp;
    }

    if (a > b)
    {
        temp = a;
        a = b;
        b = temp;
    }

    return b;
}


uint16_t Vl53l0xSensor_UpdateFilter(uint16_t rawDistanceMm)
{
    s_filterBuffer[s_filterIndex] = rawDistanceMm;
    s_filterIndex++;

    if (s_filterIndex >= VL53L0X_FILTER_SIZE)
    {
        s_filterIndex = 0;
    }

    if (s_filterCount < VL53L0X_FILTER_SIZE)
    {
        s_filterCount++;
    }

    if (s_filterCount < VL53L0X_FILTER_SIZE)
    {
        return rawDistanceMm;
    }
    else
    {
        return Vl53l0xSensor_Median3(s_filterBuffer[0], s_filterBuffer[1], s_filterBuffer[2]);
    }

}
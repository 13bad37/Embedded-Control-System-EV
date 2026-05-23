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


int32_t Filter_ApplyMovingAverage(MovingAverageFilter_t *filter, int32_t newSample, uint8_t size)
{
    filter->buffer[filter->index] = newSample;
    filter->index = (filter->index + 1) % size;

    if (filter->count < size)
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

uint16_t Vl53l0xSensor_Median5(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e)
{
    uint16_t temp;
    uint16_t arr[5] = {a, b, c, d, e};
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4 - i; j++)
        {
            if (arr[j] > arr[j+1])
            {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
    return arr[2];  // middle element
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
        return Vl53l0xSensor_Median5(
            s_filterBuffer[0], s_filterBuffer[1], s_filterBuffer[2],
            s_filterBuffer[3], s_filterBuffer[4]
        );
    }
}
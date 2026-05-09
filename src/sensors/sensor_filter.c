#include <stdbool.h>
#include <stdint.h>

#include "sensors/sensor_filter.h"


static int s_lux[FILTER_WINDOW_SIZE];
static int s_tempC[FILTER_WINDOW_SIZE];
static int s_humidPct[FILTER_WINDOW_SIZE];
static int s_distance_mm[FILTER_WINDOW_SIZE];
static int s_index = 0;
static int s_count = 0;

void SensorFilter_Init(void)
{
    s_index = 0;
    s_count = 0;
}

void SensorFilter_Update(int lux, int tempC, int humidPct, int distance_mm, SensorFilteredData_t *pOut)
{
    // Push new samples
    s_lux[s_index]         = lux;
    s_tempC[s_index]       = tempC;
    s_humidPct[s_index]    = humidPct;
    s_distance_mm[s_index] = distance_mm;

    s_index = (s_index + 1) % FILTER_WINDOW_SIZE;
    if (s_count < FILTER_WINDOW_SIZE) s_count++;


    int sumLux = 0, sumTemp = 0, sumHumid = 0, sumDist = 0;
    for (int i = 0; i < s_count; i++)
    {
        sumLux   += s_lux[i];
        sumTemp  += s_tempC[i];
        sumHumid += s_humidPct[i];
        sumDist  += s_distance_mm[i];
    }

    pOut->lux         = sumLux   / s_count;
    pOut->tempC       = sumTemp  / s_count;
    pOut->humidPct    = sumHumid / s_count;
    pOut->distance_mm = sumDist  / s_count;
}
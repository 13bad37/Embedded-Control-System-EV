#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/i2C2ShtDriver.h"
#include "sensors/ambient_light_sensor.h"
#include "sensors/sensor_manager.h"
#include "tivaware_driverlib/uart.h"
#include "utils/uartstdio.h"
#include "sensors/opt3001.h"
#include "common/project_config.h"

extern QueueHandle_t uartQueue;

#define AMBIENT_LIGHT_FILTER_SIZE       4
#define OPT3001_SAMPLE_PERIOD_MS          500

static int32_t s_rawLux;
static int32_t s_filteredLux;
static int32_t s_filterBuffer[AMBIENT_LIGHT_FILTER_SIZE];
static uint8_t s_filterIndex;
static uint8_t s_filterCount;
static bool s_hasValidSample;
static TickType_t s_lastSampleTick;

static void AmbientLightSensor_UpdateFilter(int32_t rawLux)
{
    int32_t sum;
    uint8_t i;

    s_filterBuffer[s_filterIndex] = rawLux;
    s_filterIndex++;

    if (s_filterIndex >= AMBIENT_LIGHT_FILTER_SIZE)
    {
        s_filterIndex = 0;
    }

    if (s_filterCount < AMBIENT_LIGHT_FILTER_SIZE)
    {
        s_filterCount++;
    }

    sum = 0;

    for (i = 0; i < s_filterCount; i++)
    {
        sum += s_filterBuffer[i];
    }

    s_filteredLux = sum / s_filterCount;
    s_hasValidSample = true;
}

void AmbientLightSensor_Init(void)
{
    uint8_t i;

    s_rawLux = 0;
    s_filteredLux = 0;
    s_filterIndex = 0;
    s_filterCount = 0;
    s_lastSampleTick = 0;
    s_hasValidSample = false;

    for (i = 0; i < AMBIENT_LIGHT_FILTER_SIZE; i++)
    {
        s_filterBuffer[i] = 0;
    }

    bool success;

    sensorOpt3001Init();
    UARTprintf("Testing OPT3001 Sensor:\n");
    success = sensorOpt3001Test();
    while (!success) {
        vTaskDelay(PROJECT_SYSTEM_CLOCK_HZ);
        UARTprintf("Test Failed, Trying again\n");
        sensorOpt3001Init();
        success = sensorOpt3001Test();
    }
}

void AmbientLightSensor_Sample(void)
{
    uint16_t rawData = 0;
    float convertedLux = 0;

    TickType_t currentTick;


    currentTick = xTaskGetTickCount();

    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(OPT3001_SAMPLE_PERIOD_MS)) //500ms second so 2 Hz
    {
        return;
    }

    s_lastSampleTick = currentTick;

    
    if (sensorOpt3001Read(&rawData)) 
    {
        sensorOpt3001Convert(rawData, &convertedLux);
        s_rawLux = (int32_t)convertedLux; 
        AmbientLightSensor_UpdateFilter(s_rawLux);
    }
}



bool AmbientLightSensor_GetRawLux(int32_t *lux)
{
    if (lux == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *lux = s_rawLux;

    return true;
}

bool AmbientLightSensor_GetFilteredLux(int32_t *lux)
{
    if (lux == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *lux = s_filteredLux;

    return true;
}
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/i2c2ShtDriver.h"
#include "sensors/ambient_light_sensor.h"
#include "sensors/sensor_manager.h"
#include "sensors/opt3001.h"
#include "common/project_config.h"

#define OPT3001_SAMPLE_PERIOD_MS          500


static TickType_t s_lastSampleTick;

uint16_t lux;

bool AmbientLightSensor_Init(void)
{
    /* Try once here; the manager retries later so one missing sensor can't stall u. */
    if (!sensorOpt3001Init())
    {
        return false;
    }

    return sensorOpt3001Test();
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
        SensorMsg_t msg = { .type = SENSOR_MSG_LIGHT, .value = (uint32_t)convertedLux };
        if (rawdataQueue != NULL)
        {
            xQueueSendToBack(rawdataQueue, &msg, 0);
        }
    }
}

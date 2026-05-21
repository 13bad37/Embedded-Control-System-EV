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

#define OPT3001_SAMPLE_PERIOD_MS          500

extern EvSensorDataRaw_t s_rawData;

static TickType_t s_lastSampleTick;



void AmbientLightSensor_Init(void)
{
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
        s_rawData.lux = (int32_t)convertedLux; 
    }
}

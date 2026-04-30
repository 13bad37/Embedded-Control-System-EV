#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/i2cOptDriver.h"
#include "sensors/ambient_light_sensor.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"
#include "sensors/opt3001.h"
#include "common/project_config.h"

extern QueueHandle_t uartQueue;

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
    int lux_int;

    if (sensorOpt3001Read(&rawData)) 
    {
        sensorOpt3001Convert(rawData, &convertedLux);
        lux_int = (int)convertedLux; 
        xQueueSend(uartQueue, &lux_int, 0);
    }
}
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/sht31_sensor.h"
#include "sensors/i2c2ShtDriver.h"
#include "sensors/sensor_manager.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"
#include "common/project_config.h"

extern QueueHandle_t xSensorQueue;
void Sht31Sensor_Init(void)
{
    
}

void Sht31Sensor_Sample(void)
{
    uint8_t rxData[6] = {0};
    uint16_t rawTemp, rawHumid;
    int tempC, humidPct;

    if (!writeI2C2_SHT31(0x44, 0x24, 0x00)) {
        //UARTprintf("SHT31 Write Error!\n");
        return;
    }

    //vTaskDelay(pdMS_TO_TICKS(100));


    if (readI2C2_SHT31(0x44, rxData, 6))
    {
        rawTemp  = (rxData[0] << 8) | rxData[1];
        rawHumid = (rxData[3] << 8) | rxData[4];

        tempC    = -45.0f + (175.0f * ((float)rawTemp  / 65535.0f));
        humidPct = 100.0f * ((float)rawHumid / 65535.0f);

        SensorMsg_t newmsg = { .type = MSG_SHT31, .data.sht31 = { tempC, humidPct } };
        xQueueSend(xSensorQueue, &newmsg, 0);
    }
}
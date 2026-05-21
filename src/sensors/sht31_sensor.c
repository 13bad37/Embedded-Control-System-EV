#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "sensors/sht31_sensor.h"
#include "sensors/i2c2ShtDriver.h"
#include "sensors/sensor_manager.h"
#include "tivaware_driverlib/uart.h"
#include "utils/uartstdio.h"
#include "common/project_config.h"

#define SHT31_I2C_ADDRESS               0x44
#define SHT31_FILTER_SIZE               4
#define SHT31_SAMPLE_PERIOD_MS          1000

extern QueueHandle_t xSensorQueue;

extern EvSensorDataRaw_t s_rawData;

static TickType_t s_lastSampleTick;



void Sht31Sensor_Init(void)
{

}

void Sht31Sensor_Sample(void)
{
    uint8_t rxData[6] = {0};
    uint16_t rawTemp;
    uint16_t rawHumid;

    TickType_t currentTick;


    currentTick = xTaskGetTickCount();

    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(SHT31_SAMPLE_PERIOD_MS)) //1 second so 1 Hz
    {
        return;
    }

    s_lastSampleTick = currentTick;

    if (!writeI2C2_SHT31(SHT31_I2C_ADDRESS, 0x24, 0x00))
    {
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    if (readI2C2_SHT31(SHT31_I2C_ADDRESS, rxData, 6))
    {
        rawTemp  = ((uint16_t)rxData[0] << 8) | rxData[1];
        rawHumid = ((uint16_t)rxData[3] << 8) | rxData[4];

        s_rawData.temperatureC = (int32_t)(-45.0f + (175.0f * ((float)rawTemp / 65535.0f)));
        s_rawData.humidityRh = (int32_t)(100.0f * ((float)rawHumid / 65535.0f));
    }
}

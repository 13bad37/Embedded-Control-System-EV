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

static int32_t s_rawTempC;
static int32_t s_rawHumidPct;
static int32_t s_filteredTempC;
static int32_t s_filteredHumidPct;

static int32_t s_tempFilterBuffer[SHT31_FILTER_SIZE];
static int32_t s_humidFilterBuffer[SHT31_FILTER_SIZE];
static uint8_t s_filterIndex;
static uint8_t s_filterCount;
static bool s_hasValidSample;
static TickType_t s_lastSampleTick;

static void Sht31Sensor_UpdateFilter(int32_t rawTempC, int32_t rawHumidPct)
{
    int32_t tempSum;
    int32_t humidSum;
    uint8_t i;

    s_tempFilterBuffer[s_filterIndex] = rawTempC;
    s_humidFilterBuffer[s_filterIndex] = rawHumidPct;
    s_filterIndex++;

    if (s_filterIndex >= SHT31_FILTER_SIZE)
    {
        s_filterIndex = 0;
    }

    if (s_filterCount < SHT31_FILTER_SIZE)
    {
        s_filterCount++;
    }

    tempSum = 0;
    humidSum = 0;

    for (i = 0; i < s_filterCount; i++)
    {
        tempSum += s_tempFilterBuffer[i];
        humidSum += s_humidFilterBuffer[i];
    }

    s_filteredTempC = tempSum / s_filterCount;
    s_filteredHumidPct = humidSum / s_filterCount;
    s_hasValidSample = true;
}

void Sht31Sensor_Init(void)
{
    uint8_t i;

    s_rawTempC = 0;
    s_rawHumidPct = 0;
    s_filteredTempC = 0;
    s_filteredHumidPct = 0;
    s_filterIndex = 0;
    s_filterCount = 0;
    s_hasValidSample = false;
    s_lastSampleTick = 0;

    for (i = 0; i < SHT31_FILTER_SIZE; i++)
    {
        s_tempFilterBuffer[i] = 0;
        s_humidFilterBuffer[i] = 0;
    }
}

void Sht31Sensor_Sample(void)
{
    uint8_t rxData[6] = {0};
    uint16_t rawTemp;
    uint16_t rawHumid;
    int32_t tempC;
    int32_t humidPct;
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

        tempC = (int32_t)(-45.0f + (175.0f * ((float)rawTemp / 65535.0f)));
        humidPct = (int32_t)(100.0f * ((float)rawHumid / 65535.0f));

        s_rawTempC = tempC;
        s_rawHumidPct = humidPct;

        Sht31Sensor_UpdateFilter(s_rawTempC, s_rawHumidPct);
    }
}

bool Sht31Sensor_GetRawTemperatureC(int32_t *tempC)
{
    if (tempC == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *tempC = s_rawTempC;

    return true;
}

bool Sht31Sensor_GetRawHumidityPct(int32_t *humidPct)
{
    if (humidPct == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *humidPct = s_rawHumidPct;

    return true;
}

bool Sht31Sensor_GetFilteredTemperatureC(int32_t *tempC)
{
    if (tempC == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *tempC = s_filteredTempC;

    return true;
}

bool Sht31Sensor_GetFilteredHumidityPct(int32_t *humidPct)
{
    if (humidPct == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *humidPct = s_filteredHumidPct;

    return true;
}
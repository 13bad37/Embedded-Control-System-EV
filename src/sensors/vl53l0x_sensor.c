#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "sensors/i2c2ShtDriver.h"
#include "sensors/vl53l0x_sensor.h"
#include "sensors/sensor_manager.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"
#include "common/project_config.h"

#define VL53L0X_I2C_ADDRESS             0x29
#define VL53L0X_REG_SYSRANGE_START      0x00
#define VL53L0X_REG_RESULT_INTERRUPT    0x13
#define VL53L0X_REG_RESULT_RANGE_MM     0x1E
#define VL53L0X_REG_INTERRUPT_CLEAR     0x0B

#define VL53L0X_START_CONTINUOUS        0x02
#define VL53L0X_CLEAR_INTERRUPT         0x01

#define VL53L0X_FILTER_SIZE             3
#define VL53L0X_SAMPLE_PERIOD_MS        50   // 20hz or higher  1000/50 = 20
#define VL53L0X_INVALID_DISTANCE_MM     8190

extern QueueHandle_t xSensorQueue;

static uint16_t s_rawDistanceMm;
static uint16_t s_filteredDistanceMm;
static uint16_t s_filterBuffer[VL53L0X_FILTER_SIZE];
static uint8_t s_filterIndex;
static uint8_t s_filterCount;
static bool s_hasValidSample;
static TickType_t s_lastSampleTick;


static uint16_t Vl53l0xSensor_Median3(uint16_t a, uint16_t b, uint16_t c)
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


static void Vl53l0xSensor_UpdateFilter(uint16_t rawDistanceMm)
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
        s_filteredDistanceMm = rawDistanceMm;
    }
    else
    {
        s_filteredDistanceMm = Vl53l0xSensor_Median3(s_filterBuffer[0], s_filterBuffer[1], s_filterBuffer[2]);
    }

    s_hasValidSample = true;
}


void Vl53l0xSensor_Init(void)
{
    uint8_t i;

    s_rawDistanceMm = 0;
    s_filteredDistanceMm = 0;
    s_filterIndex = 0;
    s_filterCount = 0;
    s_hasValidSample = false;
    s_lastSampleTick = 0;

    for (i = 0; i < VL53L0X_FILTER_SIZE; i++)
    {
        s_filterBuffer[i] = 0;
    }

    writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_SYSRANGE_START, VL53L0X_START_CONTINUOUS);
}

void Vl53l0xSensor_Sample(void)
{
    uint8_t rxData[2] = {0};
    uint16_t distanceMm;
    TickType_t currentTick;

    currentTick = xTaskGetTickCount();

    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(VL53L0X_SAMPLE_PERIOD_MS))
    {
        return;
    }

    s_lastSampleTick = currentTick;  //50Ms for 20Hz 



    if (readI2C_VL53L0X(VL53L0X_I2C_ADDRESS, VL53L0X_REG_RESULT_RANGE_MM, rxData, 2))
    {
        distanceMm = ((uint16_t)rxData[0] << 8) | rxData[1];

        if (distanceMm == 0 || distanceMm >= VL53L0X_INVALID_DISTANCE_MM)
        {
            return;
        }

        s_rawDistanceMm = distanceMm;
        Vl53l0xSensor_UpdateFilter(s_rawDistanceMm);

        writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT);
    }
}


bool Vl53l0xSensor_GetRawDistanceMm(uint16_t *distanceMm)
{
    if (distanceMm == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *distanceMm = s_rawDistanceMm;

    return true;
}


bool Vl53l0xSensor_GetFilteredDistanceMm(uint16_t *distanceMm)
{
    if (distanceMm == 0)
    {
        return false;
    }

    if (!s_hasValidSample)
    {
        return false;
    }

    *distanceMm = s_filteredDistanceMm;

    return true;
}
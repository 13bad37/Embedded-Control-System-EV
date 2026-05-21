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
#include "utils/uartstdio.h"
#include "common/project_config.h"

#include "motor/motor_control.h" //estop

#define VL53L0X_I2C_ADDRESS             0x29
#define VL53L0X_REG_SYSRANGE_START      0x00
#define VL53L0X_REG_RESULT_INTERRUPT    0x13
#define VL53L0X_REG_RESULT_RANGE_MM     0x1E
#define VL53L0X_REG_INTERRUPT_CLEAR     0x0B

#define VL53L0X_START_CONTINUOUS        0x02
#define VL53L0X_CLEAR_INTERRUPT         0x01

#define VL53L0X_SAMPLE_PERIOD_MS        50   // 20hz or higher  1000/50 = 20
#define VL53L0X_INVALID_DISTANCE_MM     8190

extern QueueHandle_t xSensorQueue;

EvSensorDataRaw_t raw;
static TickType_t s_lastSampleTick;





void Vl53l0xSensor_Init(void)
{
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


        raw.distanceMm = distanceMm;


        writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT);
    }
}


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
#include "rtos/message_bus.h"
#include "motor/motor_control.h"

#define VL53L0X_I2C_ADDRESS             0x29
#define VL53L0X_REG_SYSRANGE_START      0x00
#define VL53L0X_REG_RESULT_INTERRUPT    0x13
#define VL53L0X_REG_RESULT_RANGE_MM     0x1E
#define VL53L0X_REG_INTERRUPT_CLEAR     0x0B

#define VL53L0X_START_CONTINUOUS        0x02
#define VL53L0X_CLEAR_INTERRUPT         0x01

#define VL53L0X_SAMPLE_PERIOD_MS        50
#define VL53L0X_INVALID_DISTANCE_MM     8190
#define VL53L0X_ESTOP_DISTANCE_MM       300U
#define VL53L0X_NOISE_FLOOR_MM          30U

static TickType_t s_lastSampleTick;
static bool       s_estopFired = false;

void Vl53l0xSensor_Init(void)
{
    writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_SYSRANGE_START, VL53L0X_START_CONTINUOUS);
}

void Vl53l0xSensor_Sample(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t    rxData[2] = {0};
    uint16_t   distanceMm;
    TickType_t currentTick = xTaskGetTickCount();

    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(VL53L0X_SAMPLE_PERIOD_MS))
    {
        return;
    }
    s_lastSampleTick = currentTick;

    if (!readI2C_VL53L0X(VL53L0X_I2C_ADDRESS,
                          VL53L0X_REG_RESULT_RANGE_MM, rxData, 2))
    {
        return;
    }

    distanceMm = ((uint16_t)rxData[0] << 8) | rxData[1];

    /* Reject invalid readings */
    if (distanceMm == 0 || distanceMm >= VL53L0X_INVALID_DISTANCE_MM)
    {
        return;
    }

    /* Filter out sensor noise floor */
    if (distanceMm < VL53L0X_NOISE_FLOOR_MM)
    {
        return;
    }

    /* E-stop threshold — fire once, latch until clear */
    if (distanceMm < VL53L0X_ESTOP_DISTANCE_MM)
    {
        if (!s_estopFired)
        {
            s_estopFired = true;
            UARTprintf("E-STOP: distance limit %d mm\r\n", distanceMm);
            SemaphoreHandle_t sem = MessageBus_GetEstopSemaphore();
            if (sem != NULL)
            {
                xSemaphoreGive(sem);
            }
        }

        SensorMsg_t msg = { .type = SENSOR_MSG_DISTANCE, .value = (uint32_t)distanceMm };
        xQueueSendToBack(rawdataQueue, &msg, 0);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT);
    }
    else
    {
        /* Distance is clear — reset latch when system is idle or starting */
        if (MotorControl_GetState() == EV_MOTOR_STATE_IDLE ||
            MotorControl_GetState() == EV_MOTOR_STATE_STARTING)
        {
            s_estopFired = false;
        }

        SensorMsg_t msg = { .type = SENSOR_MSG_DISTANCE, .value = (uint32_t)distanceMm };
        xQueueSendToBack(rawdataQueue, &msg, 0);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT);
    }
}
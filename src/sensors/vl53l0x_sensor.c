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

#define VL53L0X_I2C_ADDRESS             0x29
#define VL53L0X_REG_SYSRANGE_START      0x00
#define VL53L0X_REG_RESULT_INTERRUPT    0x13
#define VL53L0X_REG_RESULT_RANGE_MM     0x1E
#define VL53L0X_REG_INTERRUPT_CLEAR     0x0B

#define VL53L0X_START_CONTINUOUS        0x02
#define VL53L0X_CLEAR_INTERRUPT         0x01
#define VL53L0X_NEW_SAMPLE_MASK         0x07

#define VL53L0X_SAMPLE_PERIOD_MS        20
#define VL53L0X_INVALID_DISTANCE_MM     8190
#define VL53L0X_CLOSE_DISTANCE_MM       1U

static TickType_t s_lastSampleTick;


void Vl53l0xSensor_Init(void)
{
    writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, VL53L0X_REG_SYSRANGE_START, VL53L0X_START_CONTINUOUS);
}

void Vl53l0xSensor_Sample(void)
{
    uint8_t   status = 0U;
    uint8_t   rxData[2] = {0};
    uint16_t  distanceMm;
    TickType_t currentTick = xTaskGetTickCount();
    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(VL53L0X_SAMPLE_PERIOD_MS)){return;}
    s_lastSampleTick = currentTick;

    /*
     * Only trust a completed range result. Reading 0x1E when no new sample is
     * ready was turning empty/stale register values into random E-stops.
     */
    if (!readI2C_VL53L0X(VL53L0X_I2C_ADDRESS, VL53L0X_REG_RESULT_INTERRUPT, &status, 1)){return;}
    if ((status & VL53L0X_NEW_SAMPLE_MASK) == 0U){return;}

    if (!readI2C_VL53L0X(VL53L0X_I2C_ADDRESS, VL53L0X_REG_RESULT_RANGE_MM, rxData, 2)){return;}
    distanceMm = ((uint16_t)rxData[0] << 8) | rxData[1];
    if (distanceMm == 0U)
    {
        /* Zero/very close still means blocked; don't hide it as sensor noise. */
        distanceMm = VL53L0X_CLOSE_DISTANCE_MM;
    }
    else if (distanceMm >= VL53L0X_INVALID_DISTANCE_MM)
    {
        /*
         * A completed no-target reading means nothing close is in the path.
         * Queue it so removing your hand actually clears the old blocked value.
         */
        distanceMm = VL53L0X_INVALID_DISTANCE_MM;
    }

    /* Sensor Manager owns the user threshold, so there is one E-stop rule. */
    SensorMsg_t msg = { .type = SENSOR_MSG_DISTANCE, .value = (uint32_t)distanceMm };

    if (rawdataQueue != NULL)
    {
        (void)xQueueSendToBack(rawdataQueue, &msg, 0);
    }
    (void)writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS,
                                 VL53L0X_REG_INTERRUPT_CLEAR,
                                 VL53L0X_CLEAR_INTERRUPT);
}

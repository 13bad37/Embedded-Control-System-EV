#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "sensors/sht31_sensor.h"
#include "sensors/i2c2ShtDriver.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"
#include "common/project_config.h"
#include "sensors/vl53l0x_sensor.h"
#include "sensors/sensor_manager.h"

extern QueueHandle_t xSensorQueue;
void Vl53l0xSensor_Init(void)
{
}

void Vl53l0xSensor_Sample(void)
{
    uint8_t rxData[2] = {0}; 
    uint16_t distance_mm = 0;
    //i2c address is 0x29
    // write sensor into continous mode (0x02) 
    writeI2C_VL53L0X_Byte(0x29, 0x00, 0x02);

    if (readI2C_VL53L0X(0x29, 0x1E, rxData, 2)) // 0x14 + 10 = 0x1E
    {
        distance_mm = (rxData[0] << 8) | rxData[1];
        SensorMsg_t VLMSG = { .type = MSG_VL53L0X, .data.distance_mm = distance_mm };
        xQueueSend(xSensorQueue, &VLMSG, 0);
    }
}
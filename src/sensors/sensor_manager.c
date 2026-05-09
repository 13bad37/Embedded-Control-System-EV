#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"
#include "task.h"

#include "sensors/sensor_manager.h"

#include "sensors/ambient_light_sensor.h"
#include "sensors/hall_speed_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"
#include "sensors/i2c2ShtDriver.h"

static EvSensorData_t s_rawData;
static EvSensorData_t s_filteredData;
static EvSensorThresholds_t s_thresholds;
extern QueueHandle_t xSensorQueue;

static void SensorPollingTask(void *pvParameters);
static void prvUARTTask( void *pvParameters );

TaskHandle_t xSensorTaskHandle = NULL;

QueueHandle_t     xSensorQueue;

void vSensorCallback(TimerHandle_t xTimer) {
    xTaskNotifyGive(xSensorTaskHandle);
}

    
void SensorManager_Init(void)
{
   
    xSensorQueue = xQueueCreate(5, sizeof(SensorMsg_t));

    xTaskCreate(SensorPollingTask, "SensorTask", 512, NULL, tskIDLE_PRIORITY + 2, &xSensorTaskHandle);
    TimerHandle_t xSensorTimer = xTimerCreate("SensorTimer", pdMS_TO_TICKS(1000), pdTRUE, NULL, vSensorCallback);
    xTimerStart(xSensorTimer, 0);
    xTaskCreate(prvUARTTask, "UARTTask", 512, NULL, tskIDLE_PRIORITY + 1, NULL);


}


static void SensorPollingTask(void *pvParameters)
{
    AmbientLightSensor_Init();

    while (1)
    {
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
        {
            SensorManager_Poll();
        }
    }
}

static void prvUARTTask(void *pvParameters)
{
    SensorMsg_t msg;
    UARTprintf("UartTask: Started.\n");

    while (1)
    {

        if (xQueueReceive(xSensorQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            switch (msg.type)
            {
                case MSG_LUX:
                    UARTprintf("Lux: %5d\n", msg.data.lux);
                    break;
                case MSG_SHT31:
                    UARTprintf("SHT31 | Temp: %d C | Humid: %d %%\n",
                               msg.data.sht31.tempC, msg.data.sht31.humidPct);
                    break;
                case MSG_VL53L0X:
                    UARTprintf("VL53L0X | Distance: %4d mm\n", msg.data.distance_mm);
                    break;
            }
        }
    }
}

void SensorManager_Poll(void)
{
    /*
     * Write up properly once sensor drivers are ready
     * For now this just gives the RTOS task a nice integration point.
     */
    AmbientLightSensor_Sample();
    //MotorPowerSensor_Sample();
    //HallSpeedSensor_Sample();
    Sht31Sensor_Sample();
    Vl53l0xSensor_Sample();



    SensorFilter_Update(&s_rawData, &s_filteredData);
}

EvSensorData_t SensorManager_GetRawData(void)
{
    return s_rawData;
}

EvSensorData_t SensorManager_GetFilteredData(void)
{
    return s_filteredData;
}

EvSensorThresholds_t SensorManager_GetThresholds(void)
{
    return s_thresholds;
}

void SensorManager_SetThresholds(const EvSensorThresholds_t *thresholds)
{
    if (thresholds != 0)
    {
        s_thresholds = *thresholds;
    }
}

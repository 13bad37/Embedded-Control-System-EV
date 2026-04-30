#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/sensor_manager.h"

#include "sensors/ambient_light_sensor.h"
#include "sensors/hall_speed_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"

static EvSensorData_t s_rawData;
static EvSensorData_t s_filteredData;
static EvSensorThresholds_t s_thresholds;
extern QueueHandle_t uartQueue;

static void SensorPollingTask(void *pvParameters);
static void prvUARTTask( void *pvParameters );

void SensorManager_Init(void)
{
    xTaskCreate(SensorPollingTask, "SensorTask", 128, NULL, 2, NULL);
    xTaskCreate( prvUARTTask,  "UART", 128, NULL, 1, NULL);
    //xTaskCreate( prvMotorPowerTask,  "MPT", 512, NULL, 1, NULL);
    //xTaskCreate( prvHallSpeedTask,  "HSTASK", 512, NULL, 1, NULL);
    //xTaskCreate( prvsht31Task,  "SHT31", 512, NULL, 1, NULL);
    //xTaskCreate( prvVl53l0xTask,  "Vl53l0x", 512, NULL, 1, NULL);

}

static void SensorPollingTask(void *pvParameters)
{
    AmbientLightSensor_Init();
    // MotorPowerSensor_Init();
    // HallSpeedSensor_Init();
    // Sht31Sensor_Init();
    // Vl53l0xSensor_Init();
    while (1)
    {
        SensorManager_Poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void prvUARTTask( void *pvParameters )
{
    int receivedLux;
    
    UARTprintf("UartTask: Started.\n");

    while(1)
    {
        if (xQueueReceive(uartQueue, &receivedLux, portMAX_DELAY) == pdPASS)
        {
            UARTprintf("Lux: %5d\n", receivedLux);
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
    //Sht31Sensor_Sample();
    //Vl53l0xSensor_Sample();



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

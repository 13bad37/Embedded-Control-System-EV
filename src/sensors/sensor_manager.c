#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include"queue.h"

#include "sensors/sensor_manager.h"

#include "sensors/sensor_filter.h"
#include "sensors/ambient_light_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"
#include "motor/motor_driver.h"
#include "motor/motor_state_machine.h"
#include "tivaware_driverlib/uart.h"
#include "utils/uartstdio.h"

#define SENSOR_DEFAULT_DISTANCE_MM 300U
#define SENSOR_DEFAULT_POWER_W    1800U
#define SENSOR_DEFAULT_COOLING_C  25

EvSensorDataRaw_t s_rawData;
EvSensorData_t s_filteredData;
EvSensorThresholds_t s_thresholds = {
    .max_motor_power_w = SENSOR_DEFAULT_POWER_W,
    .min_distance_mm = SENSOR_DEFAULT_DISTANCE_MM,
    .cooling_threshold_c_x10 = SENSOR_DEFAULT_COOLING_C,
    .heating_threshold_c_x10 = 0
};

MovingAverageFilter_t g_lightFilter;
MovingAverageFilter_t g_tempFilter;
MovingAverageFilter_t g_humidFilter;
MovingAverageFilter_t g_speedFilter;
MovingAverageFilter_t g_powerFilter;

QueueHandle_t rawdataQueue = NULL;

static void SensorPollingTask(void *pvParameters)
{
    AmbientLightSensor_Init();
    Sht31Sensor_Init();
    Vl53l0xSensor_Init(); 
    //These inits must run after scheduler starts so a one shot task is created to do it once (deletes after one)

    vTaskDelete(NULL);
}


void SensorManager_Init(void)
{
  
    xTaskCreate(SensorPollingTask, "SensorTask", 128, NULL, 2, NULL);
}

EvSensorDataRaw_t SensorManager_GetRawData(void)
{
    static uint32_t rawAdc0 = 0;
    static uint32_t rawAdc1 = 0;
    SensorMsg_t msg;
    while (xQueueReceive(rawdataQueue, &msg, 0) == pdTRUE)
    {
        switch (msg.type)
        {
            case SENSOR_MSG_DISTANCE: s_rawData.distanceMm = msg.value; break;
            case SENSOR_MSG_LIGHT: s_rawData.lux = msg.value; break;
            case SENSOR_MSG_TEMP: s_rawData.temperatureC = msg.value; break;
            case SENSOR_MSG_HUMID: s_rawData.humidityRh = msg.value; break;
            case SENSOR_MSG_SPEED: s_rawData.motor_speed_rpm = msg.value; break;
            case SENSOR_MSG_POWERA: 
                rawAdc0 = msg.value;
                break;

            case SENSOR_MSG_POWERB:
                rawAdc1 = msg.value;
                
                int32_t rawA = (int32_t)rawAdc0;
                int32_t rawB = (int32_t)rawAdc1;
                
                float vA = ((float)rawA / POWER_ADC_RESOLUTION) * POWER_VREF_V;
                float vB = ((float)rawB / POWER_ADC_RESOLUTION) * POWER_VREF_V;
                
                float iA = vA / (POWER_CSA_GAIN * POWER_SHUNT_OHM);
                float iB = vB / (POWER_CSA_GAIN * POWER_SHUNT_OHM);
                
                float iTotal = (iA + iB) * 1.5f; 
                s_rawData.motor_power_w = (POWER_SUPPLY_V * iTotal);
                break;
        }
    }
    return s_rawData;
}

static void updateFilter(void)
{

    s_filteredData.ambient_light_lux = Filter_ApplyMovingAverage(&g_lightFilter, s_rawData.lux, 4);
    s_filteredData.cabin_temperature_c_x10 = Filter_ApplyMovingAverage(&g_tempFilter,  s_rawData.temperatureC, 4);
    s_filteredData.cabin_humidity_rh_x10 = Filter_ApplyMovingAverage(&g_humidFilter, s_rawData.humidityRh, 4);
    s_filteredData.motor_speed_rpm = Filter_ApplyMovingAverage(&g_speedFilter, s_rawData.motor_speed_rpm, 8);
    s_filteredData.motor_power_w = Filter_ApplyMovingAverage(&g_powerFilter, s_rawData.motor_power_w, 8);
    s_filteredData.distance_mm = Vl53l0xSensor_UpdateFilter(s_rawData.distanceMm);
    
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
    if (thresholds == NULL)
    {
        return;
    }

    s_thresholds = *thresholds;
}

EvSensorThresholds_t SensorManager_CheckThresholds(void)
{

    if (s_filteredData.distance_mm < s_thresholds.min_distance_mm)
    {
        //MotorStateMachine_RequestEStop(EV_FAULT_DISTANCE_LIMIT);
    }
    
    if (s_filteredData.motor_power_w > s_thresholds.max_motor_power_w)
    {
        //MotorStateMachine_RequestEStop(EV_FAULT_DISTANCE_LIMIT);
    }
    if (s_filteredData.cabin_temperature_c_x10 < s_thresholds.cooling_threshold_c_x10)
    {
        //MotorStateMachine_RequestEStop(EV_FAULT_DISTANCE_LIMIT);
    }

    return s_thresholds;
}


void SensorManager_Poll(void)
{
    AmbientLightSensor_Sample();
    Sht31Sensor_Sample();
    Vl53l0xSensor_Sample();
    s_rawData.motor_speed_rpm = MotorDriver_GetRpm();
    updateFilter();
    SensorManager_CheckThresholds();

}

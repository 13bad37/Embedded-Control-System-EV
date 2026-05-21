#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include"queue.h"

#include "sensors/sensor_manager.h"

#include "sensors/sensor_filter.h"
#include "sensors/ambient_light_sensor.h"
#include "sensors/hall_speed_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"
#include "tivaware_driverlib/uart.h"
#include "utils/uartstdio.h"

#define EMERGENCY_DISTANCE_MM     300
#define EMERGENCY_POWER_W 1800

EvSensorDataRaw_t s_rawData;
EvSensorData_t s_filteredData;
EvSensorThresholds_t s_thresholds;

MovingAverageFilter_t g_lightFilter;
MovingAverageFilter_t g_tempFilter;
MovingAverageFilter_t g_humidFilter;
MovingAverageFilter_t g_speedFilter;
MovingAverageFilter_t g_powerFilter;

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
    return s_rawData;
}

EvSensorData_t SensorManager_GetFilteredData(void)
{
    s_filteredData.ambient_light_lux = Filter_ApplyMovingAverage(&g_lightFilter, s_rawData.lux);
    s_filteredData.cabin_temperature_c_x10 = Filter_ApplyMovingAverage(&g_tempFilter,  s_rawData.temperatureC);
    s_filteredData.cabin_humidity_rh_x10 = Filter_ApplyMovingAverage(&g_humidFilter, s_rawData.humidityRh);
    s_filteredData.motor_speed_rpm = Filter_ApplyMovingAverage(&g_speedFilter, s_rawData.motor_speed_rpm);
    s_filteredData.motor_power_w = Filter_ApplyMovingAverage(&g_powerFilter, s_rawData.motor_power_w);
    s_filteredData.distance_mm = Vl53l0xSensor_UpdateFilter(s_rawData.distanceMm);


    return s_filteredData;
}

EvSensorThresholds_t SensorManager_GetThresholds(void)
{
    return s_thresholds;
}

EvSensorThresholds_t SensorManager_CheckThresholds(void)
{

    if (s_filteredData.distance_mm < EMERGENCY_DISTANCE_MM)
    {
        //FaultManager_TriggerEStop();
        //UARTprintf("EmergencySTOP");
    }
    
    if (s_filteredData.motor_power_w > EMERGENCY_POWER_W)
    {
        //FaultManager_TriggerEStop();
    }
}

static void SensorDebug_Print(void)
{

    UARTprintf("RawLux:%d FilteredLux:%d RawTempC:%d FilteredTempC:%d RawHumidPct:%d FilteredHumidPct:%d RawDistanceMm:%d FilteredDistanceMm:%d, RawRpm:%d FilteredRpm:%d, rawPower:%d, FilteredPower:%d\n",
                s_rawData.lux,
                s_filteredData.ambient_light_lux,
                s_rawData.temperatureC,
                s_filteredData.cabin_temperature_c_x10,
                s_rawData.humidityRh,
                s_filteredData.cabin_humidity_rh_x10,
                s_rawData.distanceMm,
                s_filteredData.distance_mm,
                s_rawData.motor_speed_rpm,
                s_filteredData.motor_speed_rpm,
                s_rawData.motor_power_w,
                s_filteredData.motor_power_w);
}

void SensorManager_Poll(void)
{
    AmbientLightSensor_Sample();
    Sht31Sensor_Sample();
    Vl53l0xSensor_Sample();

    //SensorManager_CheckThresholds();
    SensorDebug_Print();
}



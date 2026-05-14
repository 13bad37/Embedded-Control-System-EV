#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include"queue.h"

#include "sensors/sensor_manager.h"

#include "sensors/ambient_light_sensor.h"
#include "sensors/hall_speed_sensor.h"
#include "sensors/motor_power_sensor.h"
#include "sensors/sensor_filter.h"
#include "sensors/sht31_sensor.h"
#include "sensors/vl53l0x_sensor.h"
#include "tivaware_driverlib/uart.h"
#include "common/uartstdio.h"

#define EMERGENCY_DISTANCE_MM     300


static EvSensorDataRaw_t s_rawData;
static EvSensorData_t s_filteredData;
static EvSensorThresholds_t s_thresholds;

static void SensorPollingTask(void *pvParameters)
{
    AmbientLightSensor_Init();
    MotorPowerSensor_Init();
    HallSpeedSensor_Init();
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
    EvSensorDataRaw_t data;
    int32_t rawLux;
    int32_t rawTempC;
    int32_t rawHumidPct;
    uint16_t rawDistanceMm;

    data = s_rawData;
    if (AmbientLightSensor_GetRawLux(&rawLux)){data.lux = rawLux;}
    if (Sht31Sensor_GetRawTemperatureC(&rawTempC)){data.temperatureC = rawTempC;}
    if (Sht31Sensor_GetRawHumidityPct(&rawHumidPct)){data.humidityRh = rawHumidPct;}
    if (Vl53l0xSensor_GetRawDistanceMm(&rawDistanceMm)){data.distanceMm = rawDistanceMm;}
    data.motor_speed_rpm = (int32_t)HallSpeedSensor_GetRawRPM();
    data.motor_power_w = (uint32_t)MotorPowerSensor_GetRawPowerW();

    s_rawData = data;
    return data;
}

EvSensorData_t SensorManager_GetFilteredData(void)
{
    EvSensorData_t data;
    int32_t filteredLux;
    int32_t filteredTempC;
    int32_t filteredHumidPct;
    uint16_t filteredDistanceMm;

    data = s_filteredData;

    if (AmbientLightSensor_GetFilteredLux(&filteredLux)){data.ambient_light_lux = filteredLux;}
    if (Sht31Sensor_GetFilteredTemperatureC(&filteredTempC)){data.cabin_temperature_c_x10 = filteredTempC;}
    if (Sht31Sensor_GetFilteredHumidityPct(&filteredHumidPct)){data.cabin_humidity_rh_x10 = filteredHumidPct;}
    if (Vl53l0xSensor_GetFilteredDistanceMm(&filteredDistanceMm)){data.distance_mm = filteredDistanceMm;}
    data.motor_speed_rpm = (int32_t)HallSpeedSensor_GetRPM();
    data.motor_power_w = (uint32_t)MotorPowerSensor_GetFilteredPowerW();
    s_filteredData = data;

    return data;
}

EvSensorThresholds_t SensorManager_GetThresholds(void)
{
    return s_thresholds;
}

EvSensorThresholds_t SensorManager_CheckThresholds(void)
{
    uint16_t filteredDist;


    if (Vl53l0xSensor_GetFilteredDistanceMm(&filteredDist))
    {
        if (filteredDist < EMERGENCY_DISTANCE_MM)
        {
            //FaultManager_TriggerEStop();
            //UARTprintf("EmergencySTOP");
        }
    }

    // if (MotorPowerSensor_IsOvercurrent())
    // {
    //     FaultManager_TriggerEStop();
    // }
}

static void SensorDebug_Print(void)
{
    int32_t rawLux;
    int32_t filteredLux;
    int32_t rawTempC;
    int32_t filteredTempC;
    int32_t rawHumidPct;
    int32_t filteredHumidPct;
    uint16_t rawDistanceMm;
    uint16_t filteredDistanceMm;
    int rawRpm = HallSpeedSensor_GetRawRPM();
    int filteredRpm = HallSpeedSensor_GetRPM();
    float rawPower = MotorPowerSensor_GetRawPowerW();
    float filteredPower = MotorPowerSensor_GetFilteredPowerW();

    if (AmbientLightSensor_GetRawLux(&rawLux) &&
        AmbientLightSensor_GetFilteredLux(&filteredLux) &&
        Sht31Sensor_GetRawTemperatureC(&rawTempC) &&
        Sht31Sensor_GetFilteredTemperatureC(&filteredTempC) &&
        Sht31Sensor_GetRawHumidityPct(&rawHumidPct) &&
        Sht31Sensor_GetFilteredHumidityPct(&filteredHumidPct) &&
        Vl53l0xSensor_GetRawDistanceMm(&rawDistanceMm) &&
        Vl53l0xSensor_GetFilteredDistanceMm(&filteredDistanceMm))
    {
        UARTprintf("RawLux:%d FilteredLux:%d RawTempC:%d FilteredTempC:%d RawHumidPct:%d FilteredHumidPct:%d RawDistanceMm:%d FilteredDistanceMm:%d RawRpm:%d FilteredRpm:%d rawPower:%d, FilteredPower:%d\n",
                   rawLux,
                   filteredLux,
                   rawTempC,
                   filteredTempC,
                   rawHumidPct,
                   filteredHumidPct,
                   rawDistanceMm,
                   filteredDistanceMm,
                   rawRpm,
                   filteredRpm,
                   rawPower,
                   filteredPower);
    }
}

void SensorManager_Poll(void)
{
    AmbientLightSensor_Sample();
    Sht31Sensor_Sample();
    Vl53l0xSensor_Sample();

    SensorManager_CheckThresholds();
    SensorDebug_Print();
}

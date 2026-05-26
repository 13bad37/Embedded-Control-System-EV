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
#include "rtos/message_bus.h"
#include "semphr.h"

#define SENSOR_DEFAULT_DISTANCE_MM 300U
#define SENSOR_DEFAULT_POWER_W    1800U
#define SENSOR_DEFAULT_COOLING_C  25
#define SENSOR_DEFAULT_HEATING_C  18
#define SENSOR_DISTANCE_TRIP_CONFIRM_SAMPLES 2U

EvSensorDataRaw_t s_rawData;
EvSensorData_t s_filteredData;
EvSensorThresholds_t s_thresholds = {
    .max_motor_power_w = SENSOR_DEFAULT_POWER_W,
    .min_distance_mm = SENSOR_DEFAULT_DISTANCE_MM,
    .cooling_threshold_c_x10 = SENSOR_DEFAULT_COOLING_C,
    .heating_threshold_c_x10 = SENSOR_DEFAULT_HEATING_C
};

MovingAverageFilter_t g_lightFilter;
MovingAverageFilter_t g_tempFilter;
MovingAverageFilter_t g_humidFilter;
MovingAverageFilter_t g_speedFilter;
MovingAverageFilter_t g_powerFilter;

QueueHandle_t rawdataQueue = NULL;
static volatile bool s_environmentReady = false;
static bool s_lightInitTried = false;
static bool s_lightReady = false;
static TickType_t s_lastLightInitTick = 0U;
static volatile bool s_lightSampleValid = false;
static volatile bool s_distanceSampleValid = false;
static uint8_t s_distanceBlockedSamples = 0U;
static bool s_distanceSafetyBlocked = false;
static volatile EvFaultCode_t s_activeFault = EV_FAULT_NONE;

static void SensorPollingTask(void *pvParameters)
{
    Sht31Sensor_Init();
    Vl53l0xSensor_Init(); 
    /* Light retries in the poll task, so its failure can't hide distance safety. */
    s_environmentReady = true;

    vTaskDelete(NULL);
}


void SensorManager_Init(void)
{
    /* This module owns the queue; one allocation avoids silently losing samples. */
    rawdataQueue = xQueueCreate(32, sizeof(SensorMsg_t));
    configASSERT(rawdataQueue != NULL);
    xTaskCreate(SensorPollingTask, "SensorTask", 128, NULL, 2, NULL);
}

static uint32_t DrainRawDataQueue(void)
{
    static uint32_t rawAdc0 = 0;
    static uint32_t rawAdc1 = 0;
    SensorMsg_t msg;
    uint32_t updated = 0U;
    while (xQueueReceive(rawdataQueue, &msg, 0) == pdTRUE)
    {
        switch (msg.type)
        {
            case SENSOR_MSG_DISTANCE:
                s_rawData.distanceMm = msg.value;
                s_distanceSampleValid = true;
                updated |= (1UL << SENSOR_MSG_DISTANCE);
                break;
            case SENSOR_MSG_LIGHT:
                s_rawData.lux = msg.value;
                s_lightSampleValid = true;
                updated |= (1UL << SENSOR_MSG_LIGHT);
                break;
            case SENSOR_MSG_TEMP:
                s_rawData.temperatureC = msg.value;
                updated |= (1UL << SENSOR_MSG_TEMP);
                break;
            case SENSOR_MSG_HUMID:
                s_rawData.humidityRh = msg.value;
                updated |= (1UL << SENSOR_MSG_HUMID);
                break;
            case SENSOR_MSG_SPEED:
                s_rawData.motor_speed_rpm = msg.value;
                updated |= (1UL << SENSOR_MSG_SPEED);
                break;
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
                updated |= (1UL << SENSOR_MSG_POWERB);
                break;
        }
    }
    return updated;
}

EvSensorDataRaw_t SensorManager_GetRawData(void)
{
    return s_rawData;
}

static void updateFilter(uint32_t updated)
{
    /* Only new readings enter each filter, so a faster safety loop adds no bias. */
    if (updated & (1UL << SENSOR_MSG_LIGHT))
    {
        s_filteredData.ambient_light_lux =
            Filter_ApplyMovingAverage(&g_lightFilter, s_rawData.lux, 4);
    }
    if (updated & (1UL << SENSOR_MSG_TEMP))
    {
        s_filteredData.cabin_temperature_c_x10 =
            Filter_ApplyMovingAverage(&g_tempFilter, s_rawData.temperatureC, 4);
    }
    if (updated & (1UL << SENSOR_MSG_HUMID))
    {
        s_filteredData.cabin_humidity_rh_x10 =
            Filter_ApplyMovingAverage(&g_humidFilter, s_rawData.humidityRh, 4);
    }
    if (updated & (1UL << SENSOR_MSG_SPEED))
    {
        s_filteredData.motor_speed_rpm =
            Filter_ApplyMovingAverage(&g_speedFilter, s_rawData.motor_speed_rpm, 8);
    }
    if (updated & (1UL << SENSOR_MSG_POWERB))
    {
        s_filteredData.motor_power_w =
            Filter_ApplyMovingAverage(&g_powerFilter, s_rawData.motor_power_w, 8);
    }
    if (updated & (1UL << SENSOR_MSG_DISTANCE))
    {
        s_filteredData.distance_mm = Vl53l0xSensor_UpdateFilter(s_rawData.distanceMm);
    }
}

static void UpdateDistanceSafetyFilter(uint32_t updated)
{
    if ((updated & (1UL << SENSOR_MSG_DISTANCE)) == 0U)
    {
        return;
    }

    if (s_rawData.distanceMm >= s_thresholds.min_distance_mm)
    {
        /*
         * One fresh clear reading releases the live gate. A fault that already
         * stopped the motor remains latched in the motor state machine.
         */
        s_distanceBlockedSamples = 0U;
        s_distanceSafetyBlocked = false;
        return;
    }

    if (s_distanceBlockedSamples < SENSOR_DISTANCE_TRIP_CONFIRM_SAMPLES)
    {
        s_distanceBlockedSamples++;
    }

    /*
     * Two fresh close ranges reject one bad ToF value but still trip quickly
     * at this sensor rate. This is the safety filter, not the GUI graph filter.
     */
    if (s_distanceBlockedSamples >= SENSOR_DISTANCE_TRIP_CONFIRM_SAMPLES)
    {
        s_distanceSafetyBlocked = true;
    }
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
    EvFaultCode_t fault = EV_FAULT_NONE;

    /* Power wins if both fail together, bc that is the motor's own limit. */
    if (s_filteredData.motor_power_w > s_thresholds.max_motor_power_w)
    {
        fault = EV_FAULT_POWER_LIMIT;
    }
    else if (s_distanceSampleValid && s_distanceSafetyBlocked)
    {
        fault = EV_FAULT_DISTANCE_LIMIT;
    }

    if (fault != s_activeFault)
    {
        s_activeFault = fault;
        if (fault != EV_FAULT_NONE)
        {
            MessageBus_RaiseEStop(fault);
        }
    }

    return s_thresholds;
}

EvFaultCode_t SensorManager_GetActiveFault(void)
{
    /* Only a confirmed live hazard blocks Start; a slow first read is not one. */
    return s_activeFault;
}

bool SensorManager_HasAmbientLightReading(void)
{
    return s_lightSampleValid;
}

void SensorManager_Poll(void)
{
    /* Do power/speed trips from boot, bc an I2C timeout mustn't hold up E-stop. */
    uint32_t updated = DrainRawDataQueue();
    updateFilter(updated);
    UpdateDistanceSafetyFilter(updated);
    SensorManager_CheckThresholds();
}

void SensorManager_PollEnvironment(void)
{
    EvMotorState_t motorState;
    bool motorActive;

    if (!s_environmentReady)
    {
        return;
    }

    /*
     * Check the safety sensor first, so failed/slow optional reads don't make
     * the distance status feel stuck when u clear an obstacle.
     */
    Vl53l0xSensor_Sample();

    motorState = MotorStateMachine_GetState();
    motorActive = (motorState == EV_MOTOR_STATE_STARTING ||
                   motorState == EV_MOTOR_STATE_RUNNING ||
                   motorState == EV_MOTOR_STATE_STOPPING ||
                   motorState == EV_MOTOR_STATE_ESTOP_BRAKING);

    TickType_t now = xTaskGetTickCount();
    if (!motorActive && !s_lightReady &&
        (!s_lightInitTried ||
         (now - s_lastLightInitTick) >= pdMS_TO_TICKS(1000U)))
    {
        /*
         * Re-probe a missing light sensor only while stopped. A failed optional
         * I2C device mustn't delay distance E-stop checks while u are running.
         */
        s_lightInitTried = true;
        s_lastLightInitTick = now;
        s_lightReady = AmbientLightSensor_Init();
    }

    /* These reads can wait on I2C, so they don't run in the fast safety loop. */
    if (s_lightReady)
    {
        AmbientLightSensor_Sample();
    }
    Sht31Sensor_Sample();
}

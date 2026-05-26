#include "rtos/tasks.h"
#include "motor/motor_driver.h"
#include "motorlib.h"
#include "FreeRTOS.h"
#include "app/fault_manager.h"
#include "app/integration.h"
#include "common/project_config.h"
#include "common/serial_plotter.h"
#include "gui/gui_manager.h"
#include "motor/motor_control.h"
#include "motor/motor_state_machine.h"
#include "rtos/message_bus.h"
#include "sensors/sensor_manager.h"
#include "task.h"
#include "utils/uartstdio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "app/adc_init.h"
#include "sensors/motor_power_sensor.h"

void MotorControlTask(void *parameters)
{
    (void)parameters;

    MotorControl_SetSpeed(250U); /* default speed, can be changed by user via GUI */

    for (;;)
    {
        EvFaultCode_t activeFault = SensorManager_GetActiveFault();
        EvFaultCode_t manualFault = FaultManager_GetActiveFault();
        EvFaultCode_t eventFault = EV_FAULT_NONE;
        EvMotorState_t currentState = MotorStateMachine_GetState();

        if (manualFault != EV_FAULT_NONE)
        {
            activeFault = manualFault;
        }

        /* A live hazard blocks Start even if its first event arrived in Idle. */
        MotorStateMachine_SetSafetyInterlock(activeFault != EV_FAULT_NONE,
                                             activeFault);

        if (MessageBus_TakeEStop(&eventFault) == pdTRUE &&
            (currentState == EV_MOTOR_STATE_STARTING ||
             currentState == EV_MOTOR_STATE_RUNNING ||
             currentState == EV_MOTOR_STATE_STOPPING))
        {
            UARTprintf("E-STOP received by motor task fault=%d\r\n", eventFault);
            MotorControl_EStop(eventFault);
        }
        else if (activeFault != EV_FAULT_NONE &&
                 (currentState == EV_MOTOR_STATE_STARTING ||
                  currentState == EV_MOTOR_STATE_RUNNING ||
                  currentState == EV_MOTOR_STATE_STOPPING))
        {
            MotorControl_EStop(activeFault);
        }

        MotorStateMachine_Run();
        MotorControl_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_MOTOR_TASK_PERIOD_MS));
    }
}

void SensorTask(void *parameters)
{
    (void)parameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    prvADCInit();

    for (;;)
    {
        /* 5 ms = 200 Hz raw power sampling, with no touch-screen trigger mix-up. */
        MotorPowerSensor_Sample();
        SensorManager_Poll();
        vTaskDelayUntil(&lastWakeTime,
                        pdMS_TO_TICKS(PROJECT_SENSOR_TASK_PERIOD_MS));
    }
}

void EnvironmentSensorTask(void *parameters)
{
    (void)parameters;
    for (;;)
    {
        SensorManager_PollEnvironment();
        vTaskDelay(pdMS_TO_TICKS(20U));
    }
}

void GuiTask(void *parameters)
{
    (void)parameters;
    for (;;)
    {
        GuiManager_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_GUI_TASK_PERIOD_MS));
    }
}

void FaultTask(void *parameters)
{
    (void)parameters;
    for (;;)
    {
        FaultManager_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_FAULT_TASK_PERIOD_MS));
    }
}

void IntegrationTask(void *parameters)
{
    (void)parameters;
    for (;;)
    {
        Integration_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_INTEGRATION_TASK_PERIOD_MS));
    }
}

void SerialPlotTask(void *parameters)
{
    (void)parameters;
    for (;;)
    {
        EvMotorStatus_t   motorStatus = MotorControl_GetStatus();
        EvSensorDataRaw_t raw         = SensorManager_GetRawData();
        EvSensorData_t    filtered    = SensorManager_GetFilteredData();

        SerialPlotter_SendFrame(&motorStatus, &raw, &filtered);
        vTaskDelay(pdMS_TO_TICKS(PROJECT_TELEMETRY_TASK_PERIOD_MS));
    }
}

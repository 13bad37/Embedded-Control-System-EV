#include "rtos/tasks.h"
#include "motor/motor_driver.h"
#include "motorlib.h"
#include "FreeRTOS.h"
#include "semphr.h"
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
#include "app/power_timer_init.h"
#include "app/adc_init.h"
#include <stdbool.h>

static bool s_hasBeenStarted = false;

void MotorControlTask(void *parameters)
{
    (void)parameters;
    SemaphoreHandle_t estopSem = MessageBus_GetEstopSemaphore();

    MotorControl_SetSpeed(250U); /* default speed, can be changed by user via GUI */

    for (;;)
    {
        if (xSemaphoreTake(estopSem, 0) == pdTRUE)
        {
            EvMotorState_t currentState = MotorControl_GetState();
            if (currentState == EV_MOTOR_STATE_RUNNING ||
                currentState == EV_MOTOR_STATE_STARTING)
            {
                UARTprintf("E-STOP received by motor task\r\n");
                MotorControl_EStop(EV_FAULT_DISTANCE_LIMIT);
                while (xSemaphoreTake(estopSem, 0) == pdTRUE) {}
            }
        }

        MotorStateMachine_Run();
        MotorControl_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_MOTOR_TASK_PERIOD_MS));
    }
}

void SensorTask(void *parameters)
{
    (void)parameters;
    prvADCInit();
    powerTimerInit();

    for (;;)
    {
        SensorManager_Poll();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_SENSOR_TASK_PERIOD_MS));
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

        SerialPlotter_SendMotorFrame(&motorStatus);
        SerialPlotter_SendSensorFrame(&raw, &filtered);
        vTaskDelay(pdMS_TO_TICKS(PROJECT_TELEMETRY_TASK_PERIOD_MS));
    }
}
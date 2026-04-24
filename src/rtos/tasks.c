#include "rtos/tasks.h"

#include "FreeRTOS.h"
#include "app/fault_manager.h"
#include "app/integration.h"
#include "common/project_config.h"
#include "common/serial_plotter.h"
#include "gui/gui_manager.h"
#include "motor/motor_control.h"
#include "motor/motor_state_machine.h"
#include "sensors/sensor_manager.h"
#include "task.h"

void MotorControlTask(void *parameters)
{
    (void)parameters;

    for (;;)
    {
        MotorStateMachine_Run();
        MotorControl_Run();
        vTaskDelay(pdMS_TO_TICKS(PROJECT_MOTOR_TASK_PERIOD_MS));
    }
}

void SensorTask(void *parameters)
{
    (void)parameters;

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
        EvMotorStatus_t motorStatus = MotorControl_GetStatus();
        EvSensorData_t raw = SensorManager_GetRawData();
        EvSensorData_t filtered = SensorManager_GetFilteredData();

        SerialPlotter_SendMotorFrame(&motorStatus);
        SerialPlotter_SendSensorFrame(&raw, &filtered);
        vTaskDelay(pdMS_TO_TICKS(PROJECT_TELEMETRY_TASK_PERIOD_MS));
    }
}

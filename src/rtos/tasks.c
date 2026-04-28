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
#include "sensors/sensor_manager.h"
#include "task.h"
#include "utils/uartstdio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"

void MotorControlTask(void *parameters)
{
    (void)parameters;
    UARTprintf("MotorControlTask started\r\n");

    uint16_t duty_value = 5U;
    const uint16_t period_value = PROJECT_MOTORLIB_PWM_PERIOD_US;
    const uint16_t target_duty = 25U;
    UARTprintf("Enabling outputs\r\n");
    MotorDriver_EnableOutputs();

    UARTprintf("Starting kickstart\r\n");
    MotorDriver_KickstartPlaceholder();

    UARTprintf("Starting ramp\r\n");
    setDuty(duty_value);

    // Ramp up
    for (;;)
    {
        if (duty_value >= target_duty)
        {
            // UARTprintf("Ramp complete - full speed\r\n");
            setDuty(target_duty);
            break;
        }
        setDuty(duty_value);
        duty_value += 3U;
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Run at full speed for 3 seconds
    UARTprintf("Running for 3 seconds\r\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Disable Hall interrupts so ISR stops overriding the brake
    UARTprintf("Disabling Hall interrupts\r\n");
    GPIOIntDisable(GPIO_PORTM_BASE, GPIO_INT_PIN_3);
    GPIOIntDisable(GPIO_PORTH_BASE, GPIO_INT_PIN_2);
    GPIOIntDisable(GPIO_PORTN_BASE, GPIO_INT_PIN_2);

    // Ramp down
    UARTprintf("Ramping down\r\n");
    while (duty_value > 0)
    {
        duty_value -= 1U;
        setDuty(duty_value);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Hard brake
    setDuty(0);
    stopMotor(0);               // soft brake first
    vTaskDelay(pdMS_TO_TICKS(100));
    stopMotor(1);               // then hard brake
    MotorDriver_DisableOutputs();
    UARTprintf("Motor stopped\r\n");

    // Normal control loop
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

#include <stdbool.h>
#include <stdint.h>
#include "app/fault_manager.h"
#include "rtos/message_bus.h"
#include "motor/motor_control.h"
#include "utils/uartstdio.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define ESTOP_MAX_DUTY_THRESHOLD    32U

static bool s_estopTriggered = false;

void FaultManager_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    UARTprintf("FaultManager_Init done\r\n");
}

void FaultManager_Run(void)
{
    /* Condition 1 — motor power limit via duty cycle */
    EvMotorStatus_t status = MotorControl_GetStatus();
    if (status.state == EV_MOTOR_STATE_RUNNING &&
        status.pwm_duty_per_mille > ESTOP_MAX_DUTY_THRESHOLD)
    {
        if (!s_estopTriggered)
        {
            s_estopTriggered = true;
            UARTprintf("E-STOP: power limit exceeded duty=%d\r\n",
                status.pwm_duty_per_mille);
            SemaphoreHandle_t sem = MessageBus_GetEstopSemaphore();
            if (sem != NULL) { xSemaphoreGive(sem); }
        }
    }

    /* Condition 2 — e-stop button PJ0 */
    bool buttonPressed = (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0);
    if (buttonPressed && !s_estopTriggered)
    {
        s_estopTriggered = true;
        UARTprintf("E-STOP: button pressed\r\n");
        SemaphoreHandle_t sem = MessageBus_GetEstopSemaphore();
        if (sem != NULL) { xSemaphoreGive(sem); }
    }

    /* Condition 3 — acknowledge button PJ1, only in fault latched */
    bool ackButton = (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0);
    if (ackButton && MotorControl_GetState() == EV_MOTOR_STATE_FAULT_LATCHED)
    {
        UARTprintf("Fault acknowledged — returning to Idle\r\n");
        MotorStateMachine_AcknowledgeFault();
        vTaskDelay(pdMS_TO_TICKS(500));  /* debounce */
    }

    /* Reset estop latch when system returns to idle */
    if (MotorControl_GetState() == EV_MOTOR_STATE_IDLE)
    {
        s_estopTriggered = false;
    }
}

void FaultManager_RecordFault(EvFaultCode_t fault)
{
    (void)fault;
}
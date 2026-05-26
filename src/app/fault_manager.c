#include <stdbool.h>
#include <stdint.h>
#include "app/fault_manager.h"
#include "rtos/message_bus.h"
#include "motor/motor_control.h"
#include "motor/motor_state_machine.h"
#include "utils/uartstdio.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static bool s_manualEstopActive = false;
static bool s_lastEstopPressed = false;
static bool s_lastAckPressed = false;

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
    bool buttonPressed = (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0);
    s_manualEstopActive = buttonPressed;
    if (buttonPressed && !s_lastEstopPressed)
    {
        UARTprintf("E-STOP: button pressed\r\n");
        MessageBus_RaiseEStop(EV_FAULT_USER_ESTOP);
    }
    s_lastEstopPressed = buttonPressed;

    bool ackButton = (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0);
    if (ackButton && !s_lastAckPressed &&
        MotorControl_GetState() == EV_MOTOR_STATE_FAULT_LATCHED)
    {
        UARTprintf("Fault acknowledged — returning to Idle\r\n");
        MotorStateMachine_AcknowledgeFault();
    }
    s_lastAckPressed = ackButton;
}

void FaultManager_RecordFault(EvFaultCode_t fault)
{
    (void)fault;
}

EvFaultCode_t FaultManager_GetActiveFault(void)
{
    return s_manualEstopActive ? EV_FAULT_USER_ESTOP : EV_FAULT_NONE;
}

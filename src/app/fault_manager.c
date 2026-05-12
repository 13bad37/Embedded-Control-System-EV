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

static bool s_estopTriggered = false;

void FaultManager_Init(void)
{
    /* Enable button GPIO — Port J onboard buttons on TM4C1294 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    UARTprintf("FaultManager_Init done\r\n");
}

void FaultManager_Run(void)
{
    /* Active low button — pressed when reads 0 */
    bool buttonPressed = (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0);

    if (buttonPressed && !s_estopTriggered)
    {
        s_estopTriggered = true;
        UARTprintf("E-STOP button pressed\r\n");

        /* Signal motor task via semaphore */
        SemaphoreHandle_t sem = MessageBus_GetEstopSemaphore();
        if (sem != NULL)
        {
            xSemaphoreGive(sem);
        }
    }

    /* Reset latch when system returns to idle */
    if (MotorControl_GetState() == EV_MOTOR_STATE_IDLE)
    {
        s_estopTriggered = false;
    }
}

void FaultManager_RecordFault(EvFaultCode_t fault)
{
    (void)fault;
    /* TODO: store/report faults once we come up with a fault model*/
}



#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "app/system_init.h"
#include "rtos/message_bus.h"
#include "rtos/task_manager.h"
#include "task.h"
#include "utils/uartstdio.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"

int main(void)
{
    SystemInit_ConfigureBoard();
    AppModules_Init();

    UARTprintf("Starting AppModules_Init\r\n");
    AppModules_Init();

    UARTprintf("Starting MessageBus_Init\r\n");
    if (MessageBus_Init() != pdPASS)
    {
        UARTprintf("ERROR: MessageBus_Init failed\r\n");
        for (;;) {}
    }

    UARTprintf("Starting RtosTasks_CreateAll\r\n");
    if (RtosTasks_CreateAll() != pdPASS)
    {
        UARTprintf("ERROR: RtosTasks_CreateAll failed\r\n");
        for (;;) {}
    }

    UARTprintf("Starting scheduler\r\n");
    vTaskStartScheduler();

    UARTprintf("ERROR: Scheduler returned - heap too small?\r\n");


    if (MessageBus_Init() != pdPASS)
    {
        /* Can replace later when we implement proper starup fault handling */
        for (;;)
        {
        }
    }

    if (RtosTasks_CreateAll() != pdPASS)
    {

        for (;;)
        {
        }
    }

    vTaskStartScheduler();


    for (;;)
    {
    }
}


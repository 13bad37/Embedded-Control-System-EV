
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

    if (MessageBus_Init() != pdPASS)
    {
        UARTprintf("ERROR: MessageBus_Init failed\r\n");
        for (;;) {}
    }

    if (RtosTasks_CreateAll() != pdPASS)
    {
        UARTprintf("ERROR: RtosTasks_CreateAll failed\r\n");
        for (;;) {}
    }

    vTaskStartScheduler();

    UARTprintf("ERROR: Scheduler returned - heap too small?\r\n");
    for (;;) {}
}


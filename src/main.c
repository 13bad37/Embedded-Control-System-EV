#include "FreeRTOS.h"
#include "app/system_init.h"
#include "rtos/message_bus.h"
#include "rtos/task_manager.h"
#include "task.h"

int main(void)
{
    SystemInit_ConfigureBoard();
    AppModules_Init();

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


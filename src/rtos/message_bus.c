#include "rtos/message_bus.h"
#include "common/events.h"
#include "common/project_config.h"
#include "semphr.h"
#include "task.h"

static QueueHandle_t    s_eventQueue   = NULL;
static SemaphoreHandle_t s_estopSem    = NULL;
static volatile EvFaultCode_t s_pendingEstopFault = EV_FAULT_NONE;

BaseType_t MessageBus_Init(void)
{
    s_eventQueue = xQueueCreate(PROJECT_EVENT_QUEUE_LENGTH, sizeof(EvEventMessage_t));
    if (s_eventQueue == NULL) { return pdFAIL; }

    s_estopSem = xSemaphoreCreateBinary();
    if (s_estopSem == NULL) { return pdFAIL; }

    return pdPASS;
}

QueueHandle_t MessageBus_GetEventQueue(void)
{
    return s_eventQueue;
}

SemaphoreHandle_t MessageBus_GetEstopSemaphore(void)
{
    return s_estopSem;
}

void MessageBus_RaiseEStop(EvFaultCode_t fault)
{
    if (s_estopSem == NULL || fault == EV_FAULT_NONE)
    {
        return;
    }

    /* Keep the cause next to the signal so the motor task doesn't have to guess. */
    taskENTER_CRITICAL();
    s_pendingEstopFault = fault;
    taskEXIT_CRITICAL();
    (void)xSemaphoreGive(s_estopSem);
}

BaseType_t MessageBus_TakeEStop(EvFaultCode_t *fault)
{
    if (fault == NULL || s_estopSem == NULL ||
        xSemaphoreTake(s_estopSem, 0) != pdTRUE)
    {
        return pdFALSE;
    }

    taskENTER_CRITICAL();
    *fault = s_pendingEstopFault;
    taskEXIT_CRITICAL();
    return pdTRUE;
}

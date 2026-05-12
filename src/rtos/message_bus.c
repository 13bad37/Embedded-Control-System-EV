#include "rtos/message_bus.h"
#include "common/events.h"
#include "common/project_config.h"
#include "semphr.h"

static QueueHandle_t    s_eventQueue   = NULL;
static SemaphoreHandle_t s_estopSem    = NULL;

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
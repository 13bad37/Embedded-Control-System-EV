#include "rtos/message_bus.h"

#include "common/events.h"
#include "common/project_config.h"

static QueueHandle_t s_eventQueue = NULL;

BaseType_t MessageBus_Init(void)
{
    s_eventQueue = xQueueCreate(PROJECT_EVENT_QUEUE_LENGTH, sizeof(EvEventMessage_t));

    if (s_eventQueue == NULL)
    {
        return pdFAIL;
    }

    return pdPASS;
}

QueueHandle_t MessageBus_GetEventQueue(void)
{
    return s_eventQueue;
}


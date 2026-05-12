#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "common/project_types.h"

BaseType_t        MessageBus_Init(void);
QueueHandle_t     MessageBus_GetEventQueue(void);
SemaphoreHandle_t MessageBus_GetEstopSemaphore(void);
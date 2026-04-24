#pragma once

#include "FreeRTOS.h"
#include "queue.h"

BaseType_t MessageBus_Init(void);
QueueHandle_t MessageBus_GetEventQueue(void);


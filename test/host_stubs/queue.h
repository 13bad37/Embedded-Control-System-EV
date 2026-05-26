#ifndef TEST_QUEUE_H
#define TEST_QUEUE_H

#include "FreeRTOS.h"

typedef void *QueueHandle_t;

BaseType_t xQueueSendToBack(QueueHandle_t queue,
                            const void *message,
                            TickType_t waitTicks);

#endif

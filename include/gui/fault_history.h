#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"
#include "gui/aest_clock.h"

#define GUI_FAULT_HISTORY_CAPACITY 7U

typedef struct
{
    EvFaultCode_t fault;
    char timestamp[GUI_AEST_TIMESTAMP_BUFFER_SIZE];
} GuiFaultHistoryEntry_t;

void GuiFaultHistory_Init(void);
bool GuiFaultHistory_Observe(bool estop_active,
                             EvFaultCode_t fault,
                             const char *timestamp);
uint32_t GuiFaultHistory_Count(void);
bool GuiFaultHistory_GetNewest(uint32_t index, GuiFaultHistoryEntry_t *entry);

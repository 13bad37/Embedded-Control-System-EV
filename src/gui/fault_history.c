#include "gui/fault_history.h"

static GuiFaultHistoryEntry_t s_entries[GUI_FAULT_HISTORY_CAPACITY];
static uint32_t s_entryCount;
static bool s_episodeRecorded;

void GuiFaultHistory_Init(void)
{
    s_entryCount = 0U;
    s_episodeRecorded = false;
}

bool GuiFaultHistory_Observe(bool estop_active,
                             EvFaultCode_t fault,
                             const char *timestamp)
{
    uint32_t index;
    uint32_t charIndex;

    if (!estop_active)
    {
        s_episodeRecorded = false;
        return false;
    }

    if (s_episodeRecorded || fault == EV_FAULT_NONE || timestamp == 0)
    {
        return false;
    }

    /* Newest stays at the top of the page; the last row falls off when full. */
    index = (s_entryCount < GUI_FAULT_HISTORY_CAPACITY) ?
            s_entryCount : (GUI_FAULT_HISTORY_CAPACITY - 1U);
    while (index > 0U)
    {
        s_entries[index] = s_entries[index - 1U];
        index--;
    }

    s_entries[0].fault = fault;
    charIndex = 0U;
    while (charIndex < (GUI_AEST_TIMESTAMP_BUFFER_SIZE - 1U) &&
           timestamp[charIndex] != '\0')
    {
        s_entries[0].timestamp[charIndex] = timestamp[charIndex];
        charIndex++;
    }
    s_entries[0].timestamp[charIndex] = '\0';
    if (s_entryCount < GUI_FAULT_HISTORY_CAPACITY)
    {
        s_entryCount++;
    }
    s_episodeRecorded = true;
    return true;
}

uint32_t GuiFaultHistory_Count(void)
{
    return s_entryCount;
}

bool GuiFaultHistory_GetNewest(uint32_t index, GuiFaultHistoryEntry_t *entry)
{
    if (entry == 0 || index >= s_entryCount)
    {
        return false;
    }

    *entry = s_entries[index];
    return true;
}

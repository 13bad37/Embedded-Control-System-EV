#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gui/fault_history.h"

static void AddFault(EvFaultCode_t fault, const char *timestamp)
{
    assert(GuiFaultHistory_Observe(true, fault, timestamp));
    assert(!GuiFaultHistory_Observe(true, fault, timestamp));
    assert(!GuiFaultHistory_Observe(false, EV_FAULT_NONE, timestamp));
}

int main(void)
{
    static const char *rolloverTimestamps[] = {
        "27/05/26 02:00:00 AEST", "27/05/26 02:00:01 AEST",
        "27/05/26 02:00:02 AEST", "27/05/26 02:00:03 AEST",
        "27/05/26 02:00:04 AEST", "27/05/26 02:00:05 AEST",
        "27/05/26 02:00:06 AEST", "27/05/26 02:00:07 AEST",
        "27/05/26 02:00:08 AEST"
    };
    GuiFaultHistoryEntry_t entry;
    uint32_t index;

    GuiFaultHistory_Init();
    assert(GuiFaultHistory_Count() == 0U);

    /* A visible latch with no resolved cause yet is not a log record. */
    assert(!GuiFaultHistory_Observe(true, EV_FAULT_NONE,
                                    "27/05/26 02:45:00 AEST"));
    assert(GuiFaultHistory_Observe(true, EV_FAULT_DISTANCE_LIMIT,
                                   "27/05/26 02:45:01 AEST"));
    assert(!GuiFaultHistory_Observe(true, EV_FAULT_DISTANCE_LIMIT,
                                    "27/05/26 02:45:02 AEST"));
    assert(GuiFaultHistory_Count() == 1U);
    assert(GuiFaultHistory_GetNewest(0U, &entry));
    assert(entry.fault == EV_FAULT_DISTANCE_LIMIT);
    assert(strcmp(entry.timestamp, "27/05/26 02:45:01 AEST") == 0);

    assert(!GuiFaultHistory_Observe(false, EV_FAULT_NONE,
                                    "27/05/26 02:45:03 AEST"));
    assert(GuiFaultHistory_Observe(true, EV_FAULT_USER_ESTOP,
                                   "27/05/26 02:45:04 AEST"));
    assert(GuiFaultHistory_Count() == 2U);
    assert(GuiFaultHistory_GetNewest(0U, &entry));
    assert(entry.fault == EV_FAULT_USER_ESTOP);
    assert(strcmp(entry.timestamp, "27/05/26 02:45:04 AEST") == 0);
    assert(GuiFaultHistory_GetNewest(1U, &entry));
    assert(entry.fault == EV_FAULT_DISTANCE_LIMIT);
    assert(!GuiFaultHistory_GetNewest(2U, &entry));

    GuiFaultHistory_Init();
    for (index = 0U; index < GUI_FAULT_HISTORY_CAPACITY + 2U; index++)
    {
        AddFault(EV_FAULT_POWER_LIMIT, rolloverTimestamps[index]);
    }

    assert(GuiFaultHistory_Count() == GUI_FAULT_HISTORY_CAPACITY);
    assert(GuiFaultHistory_GetNewest(0U, &entry));
    assert(strcmp(entry.timestamp, "27/05/26 02:00:08 AEST") == 0);
    assert(GuiFaultHistory_GetNewest(GUI_FAULT_HISTORY_CAPACITY - 1U, &entry));
    assert(strcmp(entry.timestamp, "27/05/26 02:00:02 AEST") == 0);

    return 0;
}

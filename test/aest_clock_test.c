#include <assert.h>
#include <string.h>

#include "gui/aest_clock.h"

int main(void)
{
    char timestamp[GUI_AEST_TIMESTAMP_BUFFER_SIZE];

    assert(GuiAestClock_Init("May 27 2026", "02:45:11"));
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 02:45:11 AEST") == 0);

    GuiAestClock_AdvanceSeconds(125U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 02:47:16 AEST") == 0);

    assert(GuiAestClock_Init("Dec 31 2026", "23:59:59"));
    GuiAestClock_AdvanceSeconds(1U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "01/01/27 00:00:00 AEST") == 0);

    assert(GuiAestClock_Init("Feb 28 2028", "23:59:59"));
    GuiAestClock_AdvanceSeconds(1U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "29/02/28 00:00:00 AEST") == 0);

    return 0;
}

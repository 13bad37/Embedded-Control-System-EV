#include <assert.h>
#include <string.h>

#include "gui/aest_clock.h"

int main(void)
{
    char timestamp[GUI_AEST_TIMESTAMP_BUFFER_SIZE];

    assert(GuiAestClock_Init("May 27 2026", "02:45:11"));
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 02:45:11 AM AEST") == 0);

    GuiAestClock_AdvanceSeconds(125U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 02:47:16 AM AEST") == 0);

    /* Check the half-day flip directly, bc this is the bit shown on screen. */
    assert(GuiAestClock_Init("May 27 2026", "20:11:00"));
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 08:11:00 PM AEST") == 0);

    assert(GuiAestClock_Init("May 27 2026", "12:00:00"));
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "27/05/26 12:00:00 PM AEST") == 0);

    assert(GuiAestClock_Init("Dec 31 2026", "23:59:59"));
    GuiAestClock_AdvanceSeconds(1U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "01/01/27 12:00:00 AM AEST") == 0);

    assert(GuiAestClock_Init("Feb 28 2028", "23:59:59"));
    GuiAestClock_AdvanceSeconds(1U);
    GuiAestClock_Format(timestamp, sizeof(timestamp));
    assert(strcmp(timestamp, "29/02/28 12:00:00 AM AEST") == 0);

    return 0;
}

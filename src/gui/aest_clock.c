#include "gui/aest_clock.h"

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} GuiAestTime_t;

static GuiAestTime_t s_now;

static uint8_t ParseTwoDigits(const char *text)
{
    return (uint8_t)(((uint8_t)(text[0] - '0') * 10U) +
                     (uint8_t)(text[1] - '0'));
}

static uint8_t ParseMonth(const char *month)
{
    static const char names[12][3] = {
        {'J', 'a', 'n'}, {'F', 'e', 'b'}, {'M', 'a', 'r'}, {'A', 'p', 'r'},
        {'M', 'a', 'y'}, {'J', 'u', 'n'}, {'J', 'u', 'l'}, {'A', 'u', 'g'},
        {'S', 'e', 'p'}, {'O', 'c', 't'}, {'N', 'o', 'v'}, {'D', 'e', 'c'}
    };
    uint8_t index;

    for (index = 0U; index < 12U; index++)
    {
        if (month[0] == names[index][0] &&
            month[1] == names[index][1] &&
            month[2] == names[index][2])
        {
            return (uint8_t)(index + 1U);
        }
    }
    return 0U;
}

static bool IsLeapYear(uint16_t year)
{
    return ((year % 4U) == 0U &&
            ((year % 100U) != 0U || (year % 400U) == 0U));
}

static uint8_t DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if (month == 2U && IsLeapYear(year))
    {
        return 29U;
    }
    if (month == 0U || month > 12U)
    {
        return 0U;
    }
    return days[month - 1U];
}

bool GuiAestClock_Init(const char *build_date, const char *build_time)
{
    uint8_t maxDay;

    if (build_date == 0 || build_time == 0)
    {
        return false;
    }

    s_now.month = ParseMonth(build_date);
    s_now.day = (build_date[4] == ' ') ?
                (uint8_t)(build_date[5] - '0') :
                ParseTwoDigits(&build_date[4]);
    s_now.year = (uint16_t)((uint16_t)(build_date[7] - '0') * 1000U +
                            (uint16_t)(build_date[8] - '0') * 100U +
                            (uint16_t)(build_date[9] - '0') * 10U +
                            (uint16_t)(build_date[10] - '0'));
    s_now.hour = ParseTwoDigits(build_time);
    s_now.minute = ParseTwoDigits(&build_time[3]);
    s_now.second = ParseTwoDigits(&build_time[6]);
    maxDay = DaysInMonth(s_now.year, s_now.month);

    return s_now.month != 0U && s_now.day != 0U && s_now.day <= maxDay &&
           s_now.hour < 24U && s_now.minute < 60U && s_now.second < 60U;
}

static void AdvanceOneSecond(void)
{
    s_now.second++;
    if (s_now.second < 60U)
    {
        return;
    }
    s_now.second = 0U;
    s_now.minute++;
    if (s_now.minute < 60U)
    {
        return;
    }
    s_now.minute = 0U;
    s_now.hour++;
    if (s_now.hour < 24U)
    {
        return;
    }
    s_now.hour = 0U;
    s_now.day++;
    if (s_now.day <= DaysInMonth(s_now.year, s_now.month))
    {
        return;
    }
    s_now.day = 1U;
    s_now.month++;
    if (s_now.month <= 12U)
    {
        return;
    }
    s_now.month = 1U;
    s_now.year++;
}

void GuiAestClock_AdvanceSeconds(uint32_t elapsed_seconds)
{
    while (elapsed_seconds > 0U)
    {
        AdvanceOneSecond();
        elapsed_seconds--;
    }
}

void GuiAestClock_Format(char *buffer, uint32_t buffer_size)
{
    uint8_t displayHour;
    bool isPm;

    if (buffer == 0 || buffer_size < GUI_AEST_TIMESTAMP_BUFFER_SIZE)
    {
        return;
    }

    /*
     * Show the time how we'd normally say it: 20:11 is 08:11 PM.
     * Keep this explicit bc noon/midnight are easy to accidentally flip.
     */
    isPm = (s_now.hour >= 12U);
    displayHour = (uint8_t)(s_now.hour % 12U);
    if (displayHour == 0U)
    {
        displayHour = 12U;
    }

    buffer[0] = (char)('0' + (s_now.day / 10U));
    buffer[1] = (char)('0' + (s_now.day % 10U));
    buffer[2] = '/';
    buffer[3] = (char)('0' + (s_now.month / 10U));
    buffer[4] = (char)('0' + (s_now.month % 10U));
    buffer[5] = '/';
    buffer[6] = (char)('0' + ((s_now.year / 10U) % 10U));
    buffer[7] = (char)('0' + (s_now.year % 10U));
    buffer[8] = ' ';
    buffer[9] = (char)('0' + (displayHour / 10U));
    buffer[10] = (char)('0' + (displayHour % 10U));
    buffer[11] = ':';
    buffer[12] = (char)('0' + (s_now.minute / 10U));
    buffer[13] = (char)('0' + (s_now.minute % 10U));
    buffer[14] = ':';
    buffer[15] = (char)('0' + (s_now.second / 10U));
    buffer[16] = (char)('0' + (s_now.second % 10U));
    buffer[17] = ' ';
    buffer[18] = isPm ? 'P' : 'A';
    buffer[19] = 'M';
    buffer[20] = ' ';
    buffer[21] = 'A';
    buffer[22] = 'E';
    buffer[23] = 'S';
    buffer[24] = 'T';
    buffer[25] = '\0';
}

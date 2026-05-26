#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GUI_AEST_TIMESTAMP_BUFFER_SIZE 23U

bool GuiAestClock_Init(const char *build_date, const char *build_time);
void GuiAestClock_AdvanceSeconds(uint32_t elapsed_seconds);
void GuiAestClock_Format(char *buffer, uint32_t buffer_size);

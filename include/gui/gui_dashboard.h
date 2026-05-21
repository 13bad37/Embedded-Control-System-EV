#pragma once
#include "FreeRTOS.h"
#include "task.h"

void GuiDashboard_Init(void);
void GuiDashboard_Update(void);

extern TaskHandle_t xDisplayTaskHandle;
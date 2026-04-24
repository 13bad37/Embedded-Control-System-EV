#include "gui/gui_manager.h"

#include "gui/gui_dashboard.h"
#include "gui/gui_sensor_plot.h"

void GuiManager_Init(void)
{
    GuiDashboard_Init();
    GuiSensorPlot_Init();
}

void GuiManager_Run(void)
{
    GuiDashboard_Update();
    GuiSensorPlot_Update();
}

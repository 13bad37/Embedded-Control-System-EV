#include "gui/gui_sensor_plot.h"
#include "gui/gui_dashboard.h"
#include "FreeRTOS.h"
#include "task.h"


void GuiSensorPlot_Init(void)
{
    /* Second page/tab for sensor time plots. */
}

void GuiSensorPlot_Update(void)
{  
    if (xDisplayTaskHandle != NULL)
    {
        xTaskNotifyGive(xDisplayTaskHandle);
    }
    
    /* Dashboard canvas keeps the ten-second rolling trace; this wakes its paint. */
}

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
    
    /* TODO: plot at least 5 seconds of motor/sensor history later. */
}

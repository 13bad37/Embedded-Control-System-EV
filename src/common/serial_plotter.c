#include "common/serial_plotter.h"

void SerialPlotter_Init(void)
{
    /*
     * UART setup for SerialPlot will go here once we lock in the UART pins and
     * frame format
     */
}

void SerialPlotter_SendMotorFrame(const EvMotorStatus_t *status)
{
    (void)status;
    /* TODO: send desired/reference/actual speed and PWM duty over UART. */
}

void SerialPlotter_SendSensorFrame(const EvSensorData_t *raw, const EvSensorData_t *filtered)
{
    (void)raw;
    (void)filtered;
    /* TODO: send raw and filtered sensor values for plotting. */
}


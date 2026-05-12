#include "common/serial_plotter.h"
#include "utils/uartstdio.h"
#include <stddef.h>

void SerialPlotter_Init(void)
{
    /* UART already configured by system init */
}

void SerialPlotter_SendMotorFrame(const EvMotorStatus_t *status)
{
    if (status == NULL) { return; }

    /* SerialPlot expects CSV format: value1,value2,...\n */
    UARTprintf("%d,%d,%d,%d\n",
        (int)status->desired_speed_rpm,
        (int)status->reference_speed_rpm,
        (int)status->actual_speed_rpm,
        (int)status->pwm_duty_per_mille);
}

void SerialPlotter_SendSensorFrame(const EvSensorData_t *raw, const EvSensorData_t *filtered)
{
    if (raw == NULL || filtered == NULL) { return; }
    /* TODO: add sensor channels once sensor data is available */
    (void)raw;
    (void)filtered;
}
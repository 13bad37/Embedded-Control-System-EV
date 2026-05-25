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
    //UARTprintf("%d,%d,%d,%d\n",
    //    (int)status->desired_speed_rpm,
    //    (int)status->reference_speed_rpm,
    //    (int)status->actual_speed_rpm,
    //    (int)status->pwm_duty_per_mille);
}

void SerialPlotter_SendSensorFrame(const EvSensorDataRaw_t *raw, const EvSensorData_t *filtered)
{
    if (raw == NULL || filtered == NULL) { return; }
    UARTprintf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                raw->lux,
                filtered->ambient_light_lux,
                raw->temperatureC,
                filtered->cabin_temperature_c_x10,
                raw->humidityRh,
                filtered->cabin_humidity_rh_x10,
                raw->distanceMm,
                filtered->distance_mm,
                raw->motor_speed_rpm,
                filtered->motor_speed_rpm,
                raw->motor_power_w,
                filtered->motor_power_w);
    (void)raw;
    (void)filtered;
}

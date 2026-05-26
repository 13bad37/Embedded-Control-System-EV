#include "common/serial_plotter.h"
#include "utils/uartstdio.h"
#include <stddef.h>

void SerialPlotter_Init(void)
{
    /* UART already configured by system init */
}

void SerialPlotter_SendFrame(const EvMotorStatus_t *status,
                             const EvSensorDataRaw_t *raw,
                             const EvSensorData_t *filtered)
{
    if (status == NULL || raw == NULL || filtered == NULL) { return; }

    /* One CSV row keeps every trace aligned in SerialPlot, incl raw vs filtered. */
    UARTprintf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
               (int)status->desired_speed_rpm,
               (int)status->reference_speed_rpm,
               (int)status->actual_speed_rpm,
               (int)status->pwm_duty_per_mille,
               (int)raw->lux,
               (int)filtered->ambient_light_lux,
               (int)raw->temperatureC,
               (int)filtered->cabin_temperature_c_x10,
               (int)raw->humidityRh,
               (int)filtered->cabin_humidity_rh_x10,
               (int)raw->distanceMm,
               (int)filtered->distance_mm,
               (int)raw->motor_speed_rpm,
               (int)filtered->motor_speed_rpm,
               (int)raw->motor_power_w,
               (int)filtered->motor_power_w);
}

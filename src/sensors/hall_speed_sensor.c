#include <stdint.h>
#include <stdbool.h>
#include <string.h>
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "motor/motor_driver.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "app/GPIO_Init.h" 
#include "app/hall_timer_init.h"

#include "motor/motor_driver.h""
#include "sensors/hall_speed_sensor.h"
#include "sensors/sensor_manager.h"
extern EvSensorDataRaw_t s_rawData;

void HallSpeedTimer_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
 
    // Compute raw RPM
    s_rawData.motor_speed_rpm = MotorDriver_GetRpm();
 
}


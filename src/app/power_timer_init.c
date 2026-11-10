#include <stdint.h>
#include <stdbool.h>
#include <string.h>
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
 
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "app/power_timer_init.h"
#include "common/project_config.h"
#include "sensors/motor_power_sensor.h"



void powerTimerInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2));
 
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);

    /* Legacy timer setup for 150 Hz current sampling; the active path uses SensorTask. */
    TimerLoadSet(TIMER2_BASE, TIMER_A,
                 (PROJECT_SYSTEM_CLOCK_HZ / POWER_SAMPLE_RATE_HZ) - 1U);
 
    TimerControlTrigger(TIMER2_BASE, TIMER_A, true); 
    TimerADCEventSet(TIMER2_BASE, TIMER_ADC_TIMEOUT_A); /* Fire the ADC on each tick. */

    TimerEnable(TIMER2_BASE, TIMER_A);
}

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
#include "app/hall_timer_init.h"
#include "sensors/hall_speed_sensor.h"


void HallTimerInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
 
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
 
    uint32_t loadVal = SysCtlClockGet() / HALL_SPEED_TIMER_HZ;
    TimerLoadSet(TIMER0_BASE, TIMER_A, loadVal - 1);
 
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
 
    TimerEnable(TIMER0_BASE, TIMER_A);
}
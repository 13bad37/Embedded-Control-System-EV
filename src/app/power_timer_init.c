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
#include "sensors/hall_speed_sensor.h"


void powerTimerInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));
 
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
 

    uint32_t loadVal = SysCtlClockGet() / 150 ; //every 150Hz
    TimerLoadSet(TIMER1_BASE, TIMER_A, loadVal - 1);
 
    TimerControlTrigger(TIMER1_BASE, TIMER_A, true); 
    TimerADCEventSet(TIMER1_BASE, TIMER_ADC_TIMEOUT_A); //tell to trigger which ADC

    TimerEnable(TIMER1_BASE, TIMER_A);
}
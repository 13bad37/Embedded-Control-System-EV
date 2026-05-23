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



void powerTimerInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2));
 
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
 


    TimerLoadSet(TIMER2_BASE, TIMER_A, 800000); //150hz
 
    TimerControlTrigger(TIMER2_BASE, TIMER_A, true); 
    TimerADCEventSet(TIMER2_BASE, TIMER_ADC_TIMEOUT_A); //tell to trigger which ADC

    TimerEnable(TIMER2_BASE, TIMER_A);
}
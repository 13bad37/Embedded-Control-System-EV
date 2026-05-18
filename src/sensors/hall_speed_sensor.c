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

#include "sensors/hall_speed_sensor.h"
#include "sensors/sensor_manager.h"
 
#define FILTER_WINDOW   8

static volatile uint32_t s_hallCount = 0;
 
static volatile uint32_t s_zeroTimeout = 0;

static volatile int s_rawRPM = 0;

static int s_filterBuf[8];
static int s_filterIndex = 0;
static int s_filterCount = 0;
static int s_filteredRPM = 0;

static int  prvFilterUpdate(int newSample);

void HallSpeedSensor_Init(void)
{
    memset(s_filterBuf, 0, sizeof(s_filterBuf));
    s_filterIndex  = 0;
    s_filterCount  = 0;
    s_filteredRPM  = 0;
    s_hallCount    = 0;
    s_zeroTimeout  = 0;
    s_rawRPM       = 0;
}



static int prvFilterUpdate(int newSample)
{
    s_filterBuf[s_filterIndex] = newSample;
    s_filterIndex = (s_filterIndex + 1) % FILTER_WINDOW;
    if (s_filterCount < FILTER_WINDOW) s_filterCount++;
 
    int sum = 0;
    for (int i = 0; i < s_filterCount; i++)
    {
        sum += s_filterBuf[i];
    }
    return sum / s_filterCount;
}



void HallSpeedTimer_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
 
    // Compute raw RPM
    s_rawRPM = MotorDriver_GetRpm();
 
    // Apply moving average filter
    s_filteredRPM = prvFilterUpdate(s_rawRPM);
    
}

int HallSpeedSensor_GetRPM(void)
{
    return s_filteredRPM = prvFilterUpdate(MotorDriver_GetRpm());
}
 
int HallSpeedSensor_GetRawRPM(void)
{
    return MotorDriver_GetRpm();
}
 

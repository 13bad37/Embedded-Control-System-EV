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

void HallA_ISR(void)
{
    GPIOIntClear(HALL_A_BASE, HALL_A_PIN);
    s_hallCount++;
    s_zeroTimeout = 0;
}
 
void HallB_ISR(void)
{
    GPIOIntClear(HALL_B_BASE, HALL_B_PIN);
    s_hallCount++;
    s_zeroTimeout = 0;
}
 
void HallC_ISR(void)
{
    GPIOIntClear(HALL_C_BASE, HALL_C_PIN);
    s_hallCount++;
    s_zeroTimeout = 0;
}

void HallSpeedTimer_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
 
    uint32_t counts = s_hallCount;
    s_hallCount = 0;
 
    if (counts == 0)
    {
        s_zeroTimeout++;
        if (s_zeroTimeout >= HALL_ZERO_SPEED_TIMEOUT)
        {
            // Motor has stopped — flush filter to zero
            s_rawRPM = 0;
            for (int i = 0; i < FILTER_WINDOW; i++) s_filterBuf[i] = 0;
            s_filterCount = FILTER_WINDOW;  // keep averaging valid
            s_filteredRPM = 0;
        }
        return;
    }
 
    // Compute raw RPM
    s_rawRPM = (int)((counts * HALL_SPEED_TIMER_HZ * 60)
                     / (6 * HALL_MOTOR_POLE_PAIRS));
 
    // Apply moving average filter
    s_filteredRPM = prvFilterUpdate(s_rawRPM);
    
}

int HallSpeedSensor_GetRPM(void)
{
    return s_filteredRPM;
}
 
int HallSpeedSensor_GetRawRPM(void)
{
    return s_rawRPM;
}
 

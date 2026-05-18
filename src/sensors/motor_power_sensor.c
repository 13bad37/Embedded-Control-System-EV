#include <stdint.h>
#include <stdbool.h>
#include <string.h>
 
#include "FreeRTOS.h"
#include "task.h"
 
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "tivaware_driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
 
#include "sensors/motor_power_sensor.h"
#include "app/adc_init.h"

#include "motor/motor_state_machine.h"




static volatile float s_rawPowerW = 0.0f;
 
// Moving average filter
static float    s_filterBuf[POWER_FILTER_WINDOW];
static int      s_filterIndex   = 0;
static int      s_filterCount   = 0;
static volatile float s_filteredPowerW = 0.0f;

static volatile bool s_overcurrent = false;

static float prvFilterUpdate(float newSample);

void MotorPowerSensor_Init(void)
{
    memset(s_filterBuf, 0, sizeof(s_filterBuf));
    s_filterIndex    = 0;
    s_filterCount    = 0;
    s_filteredPowerW = 0.0f;
    s_rawPowerW      = 0.0f;
    s_overcurrent    = false;

}

static float prvFilterUpdate(float newSample)
{
    s_filterBuf[s_filterIndex] = newSample;
    s_filterIndex = (s_filterIndex + 1) % POWER_FILTER_WINDOW;
    if (s_filterCount < POWER_FILTER_WINDOW) s_filterCount++;
 
    float sum = 0.0f;
    for (int i = 0; i < s_filterCount; i++)
    {
        sum += s_filterBuf[i];
    }
    return sum / (float)s_filterCount;
}

void MotorPower_ADC_ISR(void)
{
    uint32_t adcBuf[2];
 
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
 
    
    ADCSequenceDataGet(POWER_ADC_BASE, POWER_ADC_SEQUENCER, adcBuf); //read both adc samples 
    
    int32_t rawA = (int32_t)adcBuf[0] - 200;
    int32_t rawB = (int32_t)adcBuf[1] - 200; //idle offset 
   
    float vA = ((float)rawA / POWER_ADC_RESOLUTION) * POWER_VREF_V;
    float vB = ((float)rawB / POWER_ADC_RESOLUTION) * POWER_VREF_V; //convert to voltage 
 
    // Convert voltage to phase current (DRV8323 section 8.3.4.1)
    float iA = vA / (POWER_CSA_GAIN * POWER_SHUNT_OHM);
    float iB = vB / (POWER_CSA_GAIN * POWER_SHUNT_OHM);
    

    //float iTotal_limit = (motor_phase_currrent_limit_A + motor_phase_currrent_limit_B) + (motor_phase_currrent_limit_A + motor_phase_currrent_limit_B)/2;

    // Estimate total three-phase current from two measured phases
    float iTotal = (iA + iB) + (iA + iB)/2 ;
 
    // Compute raw power
    float rawPower = POWER_SUPPLY_V * iTotal;
    
    
    s_rawPowerW = rawPower;

    // Apply moving average filter
    s_filteredPowerW = prvFilterUpdate(rawPower);
 
    // Check overcurrent threshold — flag for motor control to handle
    if (s_filteredPowerW > 2000)             //need tuning
    {

        MotorStateMachine_RequestEStop(EV_FAULT_POWER_LIMIT);
        return;
        
    }
}

float MotorPowerSensor_GetFilteredPowerW(void)
{
    return s_filteredPowerW;
}
 
float MotorPowerSensor_GetRawPowerW(void)
{
    return s_rawPowerW;
}
 
bool MotorPowerSensor_IsOvercurrent(void)
{
    if (s_overcurrent)
    {
        s_overcurrent = false;   // clear after read
        return true;
    }
    return false;
}
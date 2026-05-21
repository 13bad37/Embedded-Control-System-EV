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
 
#include "sensors/sensor_manager.h"
#include "sensors/motor_power_sensor.h"
#include "app/adc_init.h"

#include "motor/motor_state_machine.h"


extern EvSensorDataRaw_t s_rawData;

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
    
    
    s_rawData.motor_power_w = rawPower;
}


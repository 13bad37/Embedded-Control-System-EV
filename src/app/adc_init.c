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

#include "app/adc_init.h"
 
#include "sensors/motor_power_sensor.h"

 


void prvADCInit(void)
{
    SysCtlPeripheralEnable(POWER_ADC_PERIPH_A);
    SysCtlPeripheralEnable(POWER_ADC_PERIPH_B);
    while (!SysCtlPeripheralReady(POWER_ADC_PERIPH_A));
    while (!SysCtlPeripheralReady(POWER_ADC_PERIPH_B));
 
    // Configure as analog inputs
    GPIOPinTypeADC(POWER_ADC_PORT_A, POWER_ADC_PIN_A);
    GPIOPinTypeADC(POWER_ADC_PORT_B, POWER_ADC_PIN_B);
 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1));
 
    // Use hardware oversampling x4 to further reduce switching noise
    // Each reported sample is average of 4 hardware conversions
    //ADCHardwareOversampleConfigure(POWER_ADC_BASE, 4);
 
    // tell the ADC to be triggered by the timer 
    ADCSequenceConfigure(POWER_ADC_BASE, POWER_ADC_SEQUENCER,
                         ADC_TRIGGER_TIMER, 0);
 
    // Step 0: sample ISENA (AIN0)
    ADCSequenceStepConfigure(POWER_ADC_BASE, POWER_ADC_SEQUENCER, 0,
                             POWER_ADC_CH_A);
 
    // Step 1: sample ISENB (AIN4), trigger interrupt, end of sequence
    ADCSequenceStepConfigure(POWER_ADC_BASE, POWER_ADC_SEQUENCER, 1,
                             POWER_ADC_CH_B | ADC_CTL_IE | ADC_CTL_END);
    
                             
    ADCIntEnable(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    IntEnable(INT_ADC1SS2);
    IntPrioritySet(INT_ADC1SS2, 0xE0);
 
    ADCSequenceEnable(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
}
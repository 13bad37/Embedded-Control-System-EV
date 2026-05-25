#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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
#include "driverlib/adc.h"

#include "motor/motor_state_machine.h"

uint32_t motor_power_w;
static uint32_t adcBuf[2];

void MotorPowerSensor_Sample(void)
{
    // Disable the touchscreen interrupt (ADC0SS3) to prevent the touchscreen
    // from switching GPIOs and causing heavy VDD/GND sag while we are sampling
    // the motor power ADCs. This perfectly eliminates the noise coupling!
    IntDisable(INT_ADC0SS3);

    ADCProcessorTrigger(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    while(!ADCIntStatus(POWER_ADC_BASE, POWER_ADC_SEQUENCER, false));
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    ADCSequenceDataGet(POWER_ADC_BASE, POWER_ADC_SEQUENCER, adcBuf); //read both adc samples

    // Re-enable the touchscreen interrupt now that analog sampling is complete
    IntEnable(INT_ADC0SS3);

    if (rawdataQueue != NULL)
    {
        SensorMsg_t msgA = { .type = SENSOR_MSG_POWERA, .value = adcBuf[0] };
        SensorMsg_t msgB = { .type = SENSOR_MSG_POWERB, .value = adcBuf[1] };

        xQueueSendToBack(rawdataQueue, &msgA, 0);
        xQueueSendToBack(rawdataQueue, &msgB, 0);
    }
}

void MotorPower_ADC_ISR(void)
{
    // Unused, polling approach is used for synchronous sampling
}

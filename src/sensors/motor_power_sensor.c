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

void MotorPower_ADC_ISR(void)
{
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    ADCSequenceDataGet(POWER_ADC_BASE, POWER_ADC_SEQUENCER, adcBuf);
    
    SensorMsg_t msg = { .type = SENSOR_MSG_POWERA, .value = (uint32_t)adcBuf[0] };
    if (rawdataQueue != NULL)
    {
        xQueueSendToBackFromISR(rawdataQueue, &msg, 0);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    SensorMsg_t msg1 = { .type = SENSOR_MSG_POWERB, .value = (uint32_t)adcBuf[1] };
    if (rawdataQueue != NULL)
    {
        xQueueSendToBackFromISR(rawdataQueue, &msg1, 0);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}



    
    
    
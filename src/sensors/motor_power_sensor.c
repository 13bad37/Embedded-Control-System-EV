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

#include "motor/motor_state_machine.h"


//extern EvSensorDataRaw_t s_rawData;

uint32_t motor_power_w;
void MotorPower_ADC_ISR(void)
{
    uint32_t adcBuf[2];
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    ADCSequenceDataGet(POWER_ADC_BASE, POWER_ADC_SEQUENCER, adcBuf); //read both adc samples 
    

    //s_rawData.motor_power_w = (uint32_t)rawPower;
    // SensorMsg_t msg = { .type = SENSOR_MSG_POWERA, .value = (uint32_t)adcBuf[0] };
    // if (rawdataQueue != NULL)
    // {
    //     xQueueSendToBack(rawdataQueue, &msg, 0);
    //     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    // }
    // SensorMsg_t msg = { .type = SENSOR_MSG_POWERB, .value = (uint32_t)adcBuf[1] };
    // if (rawdataQueue != NULL)
    // {
    //     xQueueSendToBack(rawdataQueue, &msg, 0);
    //     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    // }

}



    
    
    
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

#define POWER_ADC_POLL_LIMIT 100000U

void MotorPowerSensor_Sample(void)
{
    uint32_t pollCount = POWER_ADC_POLL_LIMIT;

    /*
     * Read ADC1 only when this function asks for it. Touch uses a timer ADC
     * path, so screen presses can't trigger fake power samples anymore.
     */
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    ADCProcessorTrigger(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    while ((ADCIntStatus(POWER_ADC_BASE, POWER_ADC_SEQUENCER, false) == 0U) &&
           (pollCount > 0U))
    {
        pollCount--;
    }

    if (pollCount == 0U)
    {
        return;
    }

    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
    ADCSequenceDataGet(POWER_ADC_BASE, POWER_ADC_SEQUENCER, adcBuf);

    if (rawdataQueue != NULL)
    {
        SensorMsg_t msgA = { .type = SENSOR_MSG_POWERA, .value = adcBuf[0] };
        SensorMsg_t msgB = { .type = SENSOR_MSG_POWERB, .value = adcBuf[1] };
        (void)xQueueSendToBack(rawdataQueue, &msgA, 0);
        (void)xQueueSendToBack(rawdataQueue, &msgB, 0);
    }
}

void MotorPower_ADC_ISR(void)
{
    /* Kept for the vector table; normal power sampling keeps this IRQ masked. */
    ADCIntClear(POWER_ADC_BASE, POWER_ADC_SEQUENCER);
}

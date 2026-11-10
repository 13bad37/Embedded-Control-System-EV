#include "app/interrupt_handlers.h"

void HallSensorEdgeIsr(void)
{
    /*
     * Hall edge capture needs to be short
     */
}

void MotorPowerAdcIsr(void)
{
    /* TODO: capture the raw ADC samples and queue/buffer them for filtering */
}

void SensorSampleTimerIsr(void)
{
    /* TODO: drive fixed rate sensor sampling. Make sure not to do filtering here pls */
}

void TouchscreenIsr(void)
{
    /*
     * Compatibility hook; the touchscreen driver owns its active ADC/interrupt path.
     */
}


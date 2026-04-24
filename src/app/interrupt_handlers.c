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
     * Task sheet said that the touchscreen needs ISr and uses 1/ADC0 
     * We'll wire this up once the GUI code works
     */
}


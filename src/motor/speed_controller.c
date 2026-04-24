#include "motor/speed_controller.h"

void SpeedController_Init(void)
{
    /* Add PI/closed loop controller state here */
}

uint16_t SpeedController_Update(uint32_t reference_rpm, uint32_t measured_rpm)
{
    (void)reference_rpm;
    (void)measured_rpm;
    /* TODO: return PWM duty once the closed loop controller is done */
    return 0U;
}


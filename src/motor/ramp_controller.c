#include "motor/ramp_controller.h"

void RampController_Init(void)
{
    
}

uint32_t RampController_Update(uint32_t current_reference_rpm,
                               uint32_t requested_rpm,
                               uint32_t elapsed_ms,
                               uint8_t estop_active)
{
    (void)requested_rpm;
    (void)elapsed_ms;
    (void)estop_active;
    /* TODO:  */
    return current_reference_rpm;
}


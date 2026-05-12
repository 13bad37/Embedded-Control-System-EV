#include "motor/ramp_controller.h"
#include "common/project_config.h"

/* RPM change allowed per millisecond */
#define RAMP_ACCEL_RATE_RPM_PER_MS   0.5f    /* gentle acceleration */
#define RAMP_DECEL_RATE_RPM_PER_MS   0.5f    /* normal deceleration */
#define RAMP_ESTOP_RATE_RPM_PER_MS   5.0f    /* emergency stop - much faster */

void RampController_Init(void)
{
    /* nothing to initialise yet */
}

uint32_t RampController_Update(uint32_t current_reference_rpm,
                               uint32_t requested_rpm,
                               uint32_t elapsed_ms,
                               uint8_t  estop_active)
{
    float current = (float)current_reference_rpm;
    float target  = (float)requested_rpm;
    float delta;

    if (estop_active)
    {
        /* Emergency deceleration - ignore requested_rpm, ramp to zero */
        delta = RAMP_ESTOP_RATE_RPM_PER_MS * (float)elapsed_ms;
        current -= delta;
        if (current < 0.0f) { current = 0.0f; }
        return (uint32_t)current;
    }

    if (current < target)
    {
        delta = RAMP_ACCEL_RATE_RPM_PER_MS * (float)elapsed_ms;
        current += delta;
        if (current > target) { current = target; }
    }
    else if (current > target)
    {
        delta = RAMP_DECEL_RATE_RPM_PER_MS * (float)elapsed_ms;
        current -= delta;
        if (current < target) { current = target; }
    }

    return (uint32_t)current;
}
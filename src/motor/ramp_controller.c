#include "motor/ramp_controller.h"
#include "common/project_config.h"

#define RAMP_ACCEL_RATE_RPM_PER_MS  (PROJECT_MOTOR_ACCEL_LIMIT_RPM_PER_S  / 1000.0f)
#define RAMP_DECEL_RATE_RPM_PER_MS  (PROJECT_MOTOR_DECEL_LIMIT_RPM_PER_S  / 1000.0f)
#define RAMP_ESTOP_RATE_RPM_PER_MS  (PROJECT_MOTOR_ESTOP_DECEL_RPM_PER_S  / 1000.0f)

void RampController_Init(void)
{
    
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
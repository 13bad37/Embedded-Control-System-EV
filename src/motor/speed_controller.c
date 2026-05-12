#include "motor/speed_controller.h"
#include "common/project_config.h"

/* Tuning parameters - adjust these during testing */
#define PI_KP               0.02f
#define PI_KI               0.01f
#define PI_INTEGRAL_MAX     10.0f
#define PI_INTEGRAL_MIN    -10.0f
#define PI_OUTPUT_MAX       50U
#define PI_OUTPUT_MIN       0U

typedef struct {
    float kp;
    float ki;
    float integral;
} PiState_t;

static PiState_t s_pi = {
    .kp       = PI_KP,
    .ki       = PI_KI,
    .integral = 0.0f
};

void SpeedController_Init(void)
{
    s_pi.integral = 0.0f;
}

void SpeedController_Reset(void)
{
    s_pi.integral = 0.0f;
}

uint16_t SpeedController_Update(uint32_t reference_rpm, uint32_t measured_rpm)
{
    float error = (float)((int32_t)reference_rpm - (int32_t)measured_rpm);

    if (error > -500.0f && error < 500.0f)
    {
        s_pi.integral += error * (PROJECT_MOTOR_TASK_PERIOD_MS / 1000.0f);
        if (s_pi.integral > PI_INTEGRAL_MAX) { s_pi.integral = PI_INTEGRAL_MAX; }
        if (s_pi.integral < PI_INTEGRAL_MIN) { s_pi.integral = PI_INTEGRAL_MIN; }
    }

    float correction = (s_pi.kp * error) + (s_pi.ki * s_pi.integral);
    float output = 25.0f + correction;   /* feedforward baseline */

    if (output > (float)PI_OUTPUT_MAX) { output = (float)PI_OUTPUT_MAX; }
    if (output < (float)PI_OUTPUT_MIN) { output = (float)PI_OUTPUT_MIN; }

    return (uint16_t)output;
}
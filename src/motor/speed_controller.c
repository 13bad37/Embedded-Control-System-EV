#include "motor/speed_controller.h"
#include "common/project_config.h"

#define PI_KP               0.01f
#define PI_KI               0.005f
#define PI_INTEGRAL_MAX     3.0f
#define PI_INTEGRAL_MIN    -3.0f
#define PI_OUTPUT_MAX       30U     /* reduced from 50 — prevents overcurrent */
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
    float feedforward = (reference_rpm > 200U) ? 15.0f : 0.0f;
    float output = feedforward + correction;


    if (output > (float)PI_OUTPUT_MAX) { output = (float)PI_OUTPUT_MAX; }
    if (output < (float)PI_OUTPUT_MIN) { output = (float)PI_OUTPUT_MIN; }

    /* DEBUG */
    /* Replace the UARTprintf debug line with this */
    UARTprintf("PI: err=%d int=%d corr=%d out=%d\r\n",
        (int)error,
        (int)(s_pi.integral * 100),   /* multiply by 100 to see 2 decimal places */
        (int)(correction * 100),
        (int)output);

    return (uint16_t)output;
}
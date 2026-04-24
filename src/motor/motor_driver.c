#include "motor/motor_driver.h"

#include "common/project_config.h"
#include "motorlib.h"

void MotorDriver_Init(void)
{
    /*
     * This will later call initMotorLib(PROJECT_MOTORLIB_PWM_PERIOD_US).
     */
    (void)PROJECT_MOTORLIB_PWM_PERIOD_US;
}

void MotorDriver_KickstartPlaceholder(void)
{
    /*
     * Assignment startup flow needs an open loop kickstart before closed loop.
     * Hook this to MotorLib once state machine logic is being written
     */
}

void MotorDriver_EnableOutputs(void)
{
    /* TODO: call enableMotor() when hardware init is ready. */
}

void MotorDriver_DisableOutputs(void)
{
    /* TODO: ^^^ disableMotor() ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
}

void MotorDriver_SetPwmDuty(uint16_t duty_per_mille)
{
    (void)duty_per_mille;
 
}

void MotorDriver_UpdateCommutationFromHall(uint8_t hall_a, uint8_t hall_b, uint8_t hall_c)
{
    (void)hall_a;
    (void)hall_b;
    (void)hall_c;
    /* TODO: */
}

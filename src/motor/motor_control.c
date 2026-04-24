#include "motor/motor_control.h"

#include "motor/motor_driver.h"
#include "motor/ramp_controller.h"
#include "motor/speed_controller.h"

static EvMotorStatus_t s_motorStatus = {
    0U,
    0U,
    0U,
    0U,
    EV_MOTOR_STATE_IDLE
};

void MotorControl_Init(void)
{
    MotorDriver_Init();
    RampController_Init();
    SpeedController_Init();
}

void MotorControl_Run(void)
{
    /* TODO: add control loop handoff later */
}

void MotorControl_Start(void)
{
    /* TODO: request Idle -> Starting through the state machine. */
}

void MotorControl_Stop(void)
{
    
}

void MotorControl_SetSpeed(uint32_t rpm)
{
    s_motorStatus.desired_speed_rpm = rpm;
    /* TODO: store the user desired RPM for the ramped reference calculation. */
}

void MotorControl_EStop(EvFaultCode_t fault)
{
    (void)fault;
    /* TODO: transition into E-Stop Braking. */
}

uint32_t MotorControl_GetSpeed(void)
{
    return s_motorStatus.actual_speed_rpm;
}

EvMotorState_t MotorControl_GetState(void)
{
    return s_motorStatus.state;
}

EvMotorStatus_t MotorControl_GetStatus(void)
{
    return s_motorStatus;
}

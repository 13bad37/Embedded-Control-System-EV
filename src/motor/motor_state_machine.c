#include "motor/motor_state_machine.h"

static EvMotorState_t s_motorState = EV_MOTOR_STATE_IDLE;

void MotorStateMachine_Init(void)
{
    s_motorState = EV_MOTOR_STATE_IDLE;
}

void MotorStateMachine_Run(void)
{
    /* TODO: implement Idle, Starting, Running, E-Stop Braking, Fault Latched. */
}

void MotorStateMachine_RequestStart(void)
{
    /* TODO: Idle -> Starting once start conditions are met. */
}

void MotorStateMachine_RequestStop(void)
{
    /* TODO: request normal deceleration back to Idle. */
}

void MotorStateMachine_RequestEStop(EvFaultCode_t fault)
{
    (void)fault;
    /* TODO: transition to E-Stop Braking. */
}

void MotorStateMachine_AcknowledgeFault(void)
{
    /* TODO: Fault Latched -> Idle only after user acknowledges fault. */
}

EvMotorState_t MotorStateMachine_GetState(void)
{
    return s_motorState;
}

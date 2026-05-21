#include "motor/motor_state_machine.h"
#include "motor/motor_driver.h"
#include "utils/uartstdio.h"
#include <stdbool.h>
#include "motorlib.h"

typedef struct {
    bool startRequested;
    bool stopRequested;
    bool eStopRequested;
    bool faultAcknowledged;
    bool hallFeedbackValid;
    uint32_t actual_rpm;
    EvFaultCode_t activeFault;
} EvMotorSmInputs_t;

static EvMotorState_t    s_motorState   = EV_MOTOR_STATE_IDLE;
static EvMotorSmInputs_t s_inputs       = {0};
static EvFaultCode_t     s_latchedFault = EV_FAULT_NONE;

static void EnterRunning(void)
{
    UARTprintf("State: RUNNING\r\n");
    MotorDriver_SetCommutationEnabled(1U);
    s_motorState = EV_MOTOR_STATE_RUNNING;
}

static void EnterEStopBraking(EvFaultCode_t fault)
{
    UARTprintf("State: ESTOP_BRAKING fault=%d\r\n", fault);
    MotorDriver_SetCommutationEnabled(0U);
    s_latchedFault = fault;
    s_motorState = EV_MOTOR_STATE_ESTOP_BRAKING;
}

static void EnterFaultLatched(void)
{
    UARTprintf("State: FAULT_LATCHED\r\n");
    MotorDriver_SetCommutationEnabled(0U);
    MotorDriver_DisableOutputs();
    s_motorState = EV_MOTOR_STATE_FAULT_LATCHED;
}

static void EnterIdle(void)
{
    UARTprintf("State: IDLE\r\n");
    MotorDriver_SetCommutationEnabled(0U);
    MotorDriver_DisableOutputs();
    s_motorState = EV_MOTOR_STATE_IDLE;
}

static void EnterStarting(void)
{
    UARTprintf("State: STARTING\r\n");
    MotorDriver_SetCommutationEnabled(1U);
    MotorDriver_EnableOutputs();
    MotorDriver_KickstartPlaceholder();
    s_inputs.hallFeedbackValid = false;
    s_motorState = EV_MOTOR_STATE_STARTING;
}

void MotorStateMachine_Init(void)
{
    s_motorState   = EV_MOTOR_STATE_IDLE;
    s_latchedFault = EV_FAULT_NONE;
}

void MotorStateMachine_Run(void)
{
    switch (s_motorState)
    {
        case EV_MOTOR_STATE_IDLE:
            if (s_inputs.startRequested)
            {
                s_inputs.startRequested = false;
                EnterStarting();
            }
            break;

        case EV_MOTOR_STATE_STARTING:
            if (s_inputs.eStopRequested)
            {
                s_inputs.eStopRequested = false;
                EnterEStopBraking(s_inputs.activeFault);
                break;
            }
            if (s_inputs.hallFeedbackValid)
            {
                EnterRunning();
            }
            break;

        case EV_MOTOR_STATE_RUNNING:
            if (s_inputs.eStopRequested)
            {
                s_inputs.eStopRequested = false;
                EnterEStopBraking(s_inputs.activeFault);
                break;
            }
            if (s_inputs.stopRequested)
            {
                s_inputs.stopRequested = false;
                EnterIdle();
            }
            break;

        case EV_MOTOR_STATE_ESTOP_BRAKING:
            if (s_inputs.actual_rpm == 0U)
            {
                EnterFaultLatched();
            }
            break;

        case EV_MOTOR_STATE_FAULT_LATCHED:
            if (s_inputs.faultAcknowledged)
            {
                s_inputs.faultAcknowledged = false;
                s_latchedFault = EV_FAULT_NONE;
                EnterIdle();
            }
            break;

        default:
            EnterIdle();
            break;
    }
}

void MotorStateMachine_RequestStart(void)
{
    s_inputs.startRequested = true;
}

void MotorStateMachine_RequestStop(void)
{
    s_inputs.stopRequested = true;
}

void MotorStateMachine_RequestEStop(EvFaultCode_t fault)
{
    s_inputs.activeFault    = fault;
    s_inputs.eStopRequested = true;
}

void MotorStateMachine_AcknowledgeFault(void)
{
    s_inputs.faultAcknowledged = true;
}

void MotorStateMachine_SetHallValid(bool valid)
{
    s_inputs.hallFeedbackValid = valid;
}

void MotorStateMachine_UpdateActualRpm(uint32_t rpm)
{
    s_inputs.actual_rpm = rpm;
}

EvMotorState_t MotorStateMachine_GetState(void)
{
    return s_motorState;
}

EvFaultCode_t MotorStateMachine_GetLatchedFault(void)
{
    return s_latchedFault;
}
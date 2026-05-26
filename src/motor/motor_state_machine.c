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
    bool safetyInterlock;
    uint32_t actual_rpm;
    EvFaultCode_t activeFault;
    EvFaultCode_t interlockFault;
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
    /* Keep commutation on while moving so the ramped brake command can work. */
    MotorDriver_SetCommutationEnabled(1U);
    s_latchedFault = fault;
    s_motorState = EV_MOTOR_STATE_ESTOP_BRAKING;
}

static void EnterFaultLatched(void)
{
    UARTprintf("State: FAULT_LATCHED\r\n");
    MotorDriver_SetPwmDuty(0U);
    MotorDriver_SetCommutationEnabled(0U);
    MotorDriver_DisableOutputs();
    s_motorState = EV_MOTOR_STATE_FAULT_LATCHED;
}

static void EnterIdle(void)
{
    UARTprintf("State: IDLE\r\n");
    MotorDriver_SetPwmDuty(0U);
    MotorDriver_SetCommutationEnabled(0U);
    MotorDriver_DisableOutputs();
    s_motorState = EV_MOTOR_STATE_IDLE;
}

static void EnterStarting(void)
{
    UARTprintf("State: STARTING\r\n");
    /* Set the state first, bc outputs are live from this point onwards. */
    s_motorState = EV_MOTOR_STATE_STARTING;
    s_inputs.hallFeedbackValid = false;
    MotorDriver_SetCommutationEnabled(1U);
    MotorDriver_EnableOutputs();
    MotorDriver_BeginKickstart();
}

static void EnterStopping(void)
{
    UARTprintf("State: STOPPING\r\n");
    /* Normal stop stays controlled; the regular 500 RPM/s ramp does the work. */
    s_motorState = EV_MOTOR_STATE_STOPPING;
}

static EvFaultCode_t RequestedFault(void)
{
    if (s_inputs.safetyInterlock && s_inputs.interlockFault != EV_FAULT_NONE)
    {
        return s_inputs.interlockFault;
    }
    return s_inputs.activeFault;
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
                if (!s_inputs.safetyInterlock)
                {
                    EnterStarting();
                }
            }
            s_inputs.eStopRequested = false;
            break;

        case EV_MOTOR_STATE_STARTING:
            if (s_inputs.eStopRequested || s_inputs.safetyInterlock)
            {
                s_inputs.eStopRequested = false;
                EnterEStopBraking(RequestedFault());
                break;
            }
            if (s_inputs.stopRequested)
            {
                s_inputs.stopRequested = false;
                EnterStopping();
                break;
            }
            if (s_inputs.hallFeedbackValid)
            {
                EnterRunning();
            }
            break;

        case EV_MOTOR_STATE_RUNNING:
            if (s_inputs.eStopRequested || s_inputs.safetyInterlock)
            {
                s_inputs.eStopRequested = false;
                EnterEStopBraking(RequestedFault());
                break;
            }
            if (s_inputs.stopRequested)
            {
                s_inputs.stopRequested = false;
                EnterStopping();
            }
            break;

        case EV_MOTOR_STATE_STOPPING:
            if (s_inputs.eStopRequested || s_inputs.safetyInterlock)
            {
                s_inputs.eStopRequested = false;
                EnterEStopBraking(RequestedFault());
                break;
            }
            if (s_inputs.actual_rpm == 0U)
            {
                EnterIdle();
            }
            break;

        case EV_MOTOR_STATE_ESTOP_BRAKING:
            s_inputs.startRequested = false;
            s_inputs.stopRequested = false;
            if (s_inputs.actual_rpm == 0U)
            {
                EnterFaultLatched();
            }
            break;

        case EV_MOTOR_STATE_FAULT_LATCHED:
            s_inputs.startRequested = false;
            s_inputs.stopRequested = false;
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
    if (s_motorState == EV_MOTOR_STATE_IDLE)
    {
        s_inputs.startRequested = true;
    }
}

void MotorStateMachine_RequestStop(void)
{
    if (s_motorState == EV_MOTOR_STATE_STARTING ||
        s_motorState == EV_MOTOR_STATE_RUNNING)
    {
        s_inputs.stopRequested = true;
    }
}

void MotorStateMachine_RequestEStop(EvFaultCode_t fault)
{
    if (s_motorState == EV_MOTOR_STATE_STARTING ||
        s_motorState == EV_MOTOR_STATE_RUNNING ||
        s_motorState == EV_MOTOR_STATE_STOPPING)
    {
        s_inputs.activeFault    = fault;
        s_inputs.eStopRequested = true;
    }
}

void MotorStateMachine_AcknowledgeFault(void)
{
    if (s_motorState == EV_MOTOR_STATE_FAULT_LATCHED)
    {
        s_inputs.faultAcknowledged = true;
    }
}

void MotorStateMachine_SetHallValid(bool valid)
{
    s_inputs.hallFeedbackValid = valid;
}

void MotorStateMachine_SetSafetyInterlock(bool active, EvFaultCode_t fault)
{
    s_inputs.safetyInterlock = active;
    s_inputs.interlockFault = active ? fault : EV_FAULT_NONE;
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

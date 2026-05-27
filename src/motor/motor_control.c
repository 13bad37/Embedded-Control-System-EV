#include "FreeRTOS.h"
#include "queue.h"

#include "motor/motor_control.h"
#include "motor/motor_driver.h"
#include "motor/motor_state_machine.h"
#include "motor/ramp_controller.h"
#include "motor/speed_controller.h"
#include "common/project_config.h"
#include "utils/uartstdio.h"
#include "sensors/sensor_manager.h"

static EvMotorStatus_t s_motorStatus = {
    0U, 0U, 0U, 0U,
    EV_MOTOR_STATE_IDLE
};

void MotorControl_Init(void)
{
    MotorDriver_Init();
    RampController_Init();
    SpeedController_Init();
    MotorStateMachine_Init();
    UARTprintf("MotorControl_Init done\r\n");
}

void MotorControl_Run(void)
{
    static EvMotorState_t s_previousState = EV_MOTOR_STATE_IDLE;
    EvMotorState_t state = MotorStateMachine_GetState();

    /* A zero RPM reading is real data, especially when deciding a stop is done. */
    uint32_t rpm = MotorDriver_GetRpm();
    s_motorStatus.actual_speed_rpm = rpm;
    if (rawdataQueue != NULL)
    {
        SensorMsg_t msg = { .type = SENSOR_MSG_SPEED, .value = rpm };
        (void)xQueueSendToBack(rawdataQueue, &msg, 0);
    }

    MotorStateMachine_UpdateActualRpm(s_motorStatus.actual_speed_rpm);

    if (state == EV_MOTOR_STATE_STARTING)
    {
        MotorDriver_RunKickstart(PROJECT_MOTOR_TASK_PERIOD_MS);
        if (MotorDriver_GetHallEdgeCount() >= HALL_VALID_EDGE_THRESHOLD)
        {
            MotorStateMachine_SetHallValid(true);
        }
        else if (MotorDriver_KickstartTimedOut())
        {
            MotorControl_EStop(EV_FAULT_STARTUP);
        }
    }

    if (state == EV_MOTOR_STATE_RUNNING &&
        s_previousState == EV_MOTOR_STATE_STARTING)
    {
        /* Pick up from the kickstart speed once, then let zero stay zero. */
        s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
        SpeedController_Reset();
    }
    else if (state == EV_MOTOR_STATE_STOPPING &&
             s_previousState != EV_MOTOR_STATE_STOPPING)
    {
        /*
         * The Stop button can be pressed while the motor is still speeding up.
         * Never start a stop ramp from a target above the speed it has reached.
         */
        if (s_motorStatus.reference_speed_rpm > s_motorStatus.actual_speed_rpm)
        {
            s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
        }
        SpeedController_Reset();
    }
    else if (state == EV_MOTOR_STATE_ESTOP_BRAKING &&
             s_previousState != EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
        SpeedController_Reset();
    }

    if (state == EV_MOTOR_STATE_RUNNING ||
        state == EV_MOTOR_STATE_STOPPING)
    {
        uint32_t target = (state == EV_MOTOR_STATE_RUNNING) ?
                          s_motorStatus.desired_speed_rpm : 0U;

        s_motorStatus.reference_speed_rpm = RampController_Update(
            s_motorStatus.reference_speed_rpm,
            target,
            PROJECT_MOTOR_TASK_PERIOD_MS,
            0U
        );

        uint16_t commandedDuty;

        commandedDuty = SpeedController_Update(
            s_motorStatus.reference_speed_rpm,
            s_motorStatus.actual_speed_rpm
        );

        s_motorStatus.pwm_duty_per_mille = commandedDuty;
        MotorDriver_SetPwmDuty(s_motorStatus.pwm_duty_per_mille);
    }
    else if (state == EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        /*
         * Keep the specified emergency reference ramp for reporting, but do
         * not overwrite the brake with forward PWM while the rotor slows.
         */
        s_motorStatus.reference_speed_rpm = RampController_Update(
            s_motorStatus.reference_speed_rpm,
            0U,
            PROJECT_MOTOR_TASK_PERIOD_MS,
            1U
        );
        s_motorStatus.pwm_duty_per_mille = 0U;
    }
    else
    {
        SpeedController_Reset();
        s_motorStatus.reference_speed_rpm = 0U;
        s_motorStatus.pwm_duty_per_mille = 0U;
    }

    s_motorStatus.state = state;
    s_previousState = state;
}

void MotorControl_Start(void)
{
    if (s_motorStatus.desired_speed_rpm > 0U)
    {
        MotorStateMachine_RequestStart();
    }
}

void MotorControl_Stop(void)
{
    s_motorStatus.desired_speed_rpm = 0U;
    MotorStateMachine_RequestStop();
}

void MotorControl_SetSpeed(uint32_t rpm)
{
    EvMotorState_t state = MotorStateMachine_GetState();
    if (state == EV_MOTOR_STATE_ESTOP_BRAKING ||
        state == EV_MOTOR_STATE_FAULT_LATCHED ||
        state == EV_MOTOR_STATE_STOPPING)
    {
        return;
    }

    /*
     * non 0 speed request stays in the same range so another caller can't go past 1500rpm
     */
    if (rpm != 0U && rpm < PROJECT_MOTOR_DESIRED_MIN_RPM)
    {
        rpm = PROJECT_MOTOR_DESIRED_MIN_RPM;
    }
    else if (rpm > PROJECT_MOTOR_DESIRED_MAX_RPM)
    {
        rpm = PROJECT_MOTOR_DESIRED_MAX_RPM;
    }

    s_motorStatus.desired_speed_rpm = rpm;
    if (rpm == 0U &&
        (state == EV_MOTOR_STATE_RUNNING || state == EV_MOTOR_STATE_STARTING))
    {
        MotorStateMachine_RequestStop();
    }
}

void MotorControl_EStop(EvFaultCode_t fault)
{
    s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
    SpeedController_Reset();
    MotorStateMachine_RequestEStop(fault);
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

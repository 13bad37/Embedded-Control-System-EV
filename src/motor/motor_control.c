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
    else if (state == EV_MOTOR_STATE_ESTOP_BRAKING &&
             s_previousState != EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
        SpeedController_Reset();
    }

    if (state == EV_MOTOR_STATE_RUNNING ||
        state == EV_MOTOR_STATE_STOPPING ||
    state == EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        uint8_t estop = (state == EV_MOTOR_STATE_ESTOP_BRAKING) ? 1U : 0U;
        uint32_t target = (state == EV_MOTOR_STATE_RUNNING) ?
                          s_motorStatus.desired_speed_rpm : 0U;

        s_motorStatus.reference_speed_rpm = RampController_Update(
            s_motorStatus.reference_speed_rpm,
            target,
            PROJECT_MOTOR_TASK_PERIOD_MS,
            estop
        );

        if (state == EV_MOTOR_STATE_ESTOP_BRAKING)
        {
            /*
             * Don't run the speed PI during E-stop: if RPM falls quickly it
             * tries to drive the motor back up, which is the random tweak u saw.
             */
            uint32_t ref = s_motorStatus.reference_speed_rpm;
            s_motorStatus.pwm_duty_per_mille =
                (ref > 0U) ? (uint16_t)((ref * 15U) / 1500U) : 0U;
        }
        else
        {
            s_motorStatus.pwm_duty_per_mille = SpeedController_Update(
                s_motorStatus.reference_speed_rpm,
                s_motorStatus.actual_speed_rpm
            );
        }

        MotorDriver_SetPwmDuty(s_motorStatus.pwm_duty_per_mille);
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

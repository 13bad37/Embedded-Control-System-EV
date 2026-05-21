#include "motor/motor_control.h"
#include "motor/motor_driver.h"
#include "motor/motor_state_machine.h"
#include "motor/ramp_controller.h"
#include "motor/speed_controller.h"
#include "common/project_config.h"
#include "utils/uartstdio.h"

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
    static uint32_t s_estopTickCount = 0U;
    EvMotorState_t state = MotorStateMachine_GetState();

    /* Only update actual RPM if reading is valid */
    uint32_t rpm = MotorDriver_GetRpm();
    if (rpm > 0U)
    {
        s_motorStatus.actual_speed_rpm = rpm;
    }

    /* During estop braking, force RPM to 0 after 3 seconds
       in case hall sensor keeps reporting stale speed */
    if (state == EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        s_estopTickCount++;
        if (s_estopTickCount > (3000U / PROJECT_MOTOR_TASK_PERIOD_MS))
        {
            s_motorStatus.actual_speed_rpm = 0U;
            s_motorStatus.reference_speed_rpm = 0U;
        }
    }
    else
    {
        s_estopTickCount = 0U;
    }

    MotorStateMachine_UpdateActualRpm(s_motorStatus.actual_speed_rpm);

    if (state == EV_MOTOR_STATE_STARTING)
    {
        if (MotorDriver_GetHallEdgeCount() >= HALL_VALID_EDGE_THRESHOLD)
        {
            MotorStateMachine_SetHallValid(true);
        }
    }

    if (state == EV_MOTOR_STATE_RUNNING ||
    state == EV_MOTOR_STATE_ESTOP_BRAKING)
    {
        if (s_motorStatus.reference_speed_rpm == 0U &&
            state == EV_MOTOR_STATE_RUNNING)
        {
            s_motorStatus.reference_speed_rpm = s_motorStatus.actual_speed_rpm;
            SpeedController_Reset();
        }

        uint8_t estop = (state == EV_MOTOR_STATE_ESTOP_BRAKING) ? 1U : 0U;

        s_motorStatus.reference_speed_rpm = RampController_Update(
            s_motorStatus.reference_speed_rpm,
            s_motorStatus.desired_speed_rpm,
            PROJECT_MOTOR_TASK_PERIOD_MS,
            estop
        );

        if (estop)
        {
            /* During estop just map reference directly to duty
               bypassing the PI to avoid oscillation */
            uint32_t ref = s_motorStatus.reference_speed_rpm;
            uint16_t duty = (ref > 0U) ?
                (uint16_t)((ref * 15U) / 1500U) : 0U;
            s_motorStatus.pwm_duty_per_mille = duty;
        }
        else
        {
            s_motorStatus.pwm_duty_per_mille = SpeedController_Update(
                s_motorStatus.reference_speed_rpm,
                s_motorStatus.actual_speed_rpm
            );
            // UARTprintf("actual=%d ref=%d\r\n",
            //     s_motorStatus.actual_speed_rpm,
            //     s_motorStatus.reference_speed_rpm);
        }

        MotorDriver_SetPwmDuty(s_motorStatus.pwm_duty_per_mille);
    }
    else
    {
        SpeedController_Reset();
        s_motorStatus.reference_speed_rpm = 0U;
        s_motorStatus.actual_speed_rpm    = 0U;
    }

    s_motorStatus.state = state;
}

void MotorControl_Start(void)
{
    MotorStateMachine_RequestStart();
}

void MotorControl_Stop(void)
{
    MotorStateMachine_RequestStop();
}

void MotorControl_SetSpeed(uint32_t rpm)
{
    s_motorStatus.desired_speed_rpm = rpm;
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
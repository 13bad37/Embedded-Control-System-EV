#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "common/project_config.h"
#include "motor/motor_driver.h"
#include "motor/ramp_controller.h"
#include "motor/speed_controller.h"
#include "sensors/sensor_manager.h"

static uint32_t s_fakeRpm = 420U;
static uint16_t s_lastDuty;
static uint8_t s_commutationEnabled;
static unsigned int s_brakeCalls;
static unsigned int s_disableCalls;

QueueHandle_t rawdataQueue = NULL;

void MotorDriver_Init(void) {}
void MotorDriver_BeginKickstart(void) {}
void MotorDriver_RunKickstart(uint32_t elapsed_ms) { (void)elapsed_ms; }
uint8_t MotorDriver_KickstartTimedOut(void) { return 0U; }
void MotorDriver_EnableOutputs(void) {}
void MotorDriver_DisableOutputs(void) { s_disableCalls++; }
void MotorDriver_SetPwmDuty(uint16_t duty_per_mille) { s_lastDuty = duty_per_mille; }
void MotorDriver_UpdateCommutationFromHall(uint8_t a, uint8_t b, uint8_t c)
{
    (void)a;
    (void)b;
    (void)c;
}
uint32_t MotorDriver_GetHallEdgeCount(void) { return HALL_VALID_EDGE_THRESHOLD; }
uint32_t MotorDriver_GetRpm(void) { return s_fakeRpm; }
void MotorDriver_ResetHallState(void) {}
void MotorDriver_SetCommutationEnabled(uint8_t enabled) { s_commutationEnabled = enabled; }
void MotorDriver_ApplyEmergencyBrake(void) { s_brakeCalls++; }

void RampController_Init(void) {}
uint32_t RampController_Update(uint32_t current,
                               uint32_t target,
                               uint32_t elapsed_ms,
                               uint8_t estop)
{
    (void)elapsed_ms;
    if (estop)
    {
        return current > 10U ? current - 10U : 0U;
    }
    if (current > target)
    {
        return current > 5U ? current - 5U : target;
    }
    return target;
}

void SpeedController_Init(void) {}
void SpeedController_Reset(void) {}
uint16_t SpeedController_Update(uint32_t reference_rpm, uint32_t measured_rpm)
{
    return reference_rpm > measured_rpm ? 20U : 0U;
}

BaseType_t xQueueSendToBack(QueueHandle_t queue,
                            const void *message,
                            TickType_t wait_ticks)
{
    (void)queue;
    (void)message;
    (void)wait_ticks;
    return pdTRUE;
}

int UARTprintf(const char *format, ...)
{
    (void)format;
    return 0;
}

#include "../src/motor/motor_state_machine.c"
#include "../src/motor/motor_control.c"

int main(void)
{
    MotorControl_Init();
    MotorControl_SetSpeed(400U);
    MotorControl_Start();
    MotorStateMachine_Run();
    MotorStateMachine_SetHallValid(true);
    MotorStateMachine_Run();
    s_fakeRpm = 100U;
    MotorControl_Run();
    assert(s_motorStatus.reference_speed_rpm > s_fakeRpm);

    unsigned int disablesBeforeStop = s_disableCalls;
    MotorControl_Stop();
    MotorStateMachine_Run();
    assert(MotorStateMachine_GetState() == EV_MOTOR_STATE_IDLE);
    assert(s_lastDuty == 0U);
    assert(s_commutationEnabled == 0U);
    assert(s_disableCalls == disablesBeforeStop + 1U);
    assert(s_brakeCalls == 0U);
    MotorControl_Run();
    assert(s_lastDuty == 0U);

    s_fakeRpm = 420U;
    MotorControl_SetSpeed(400U);
    MotorControl_Start();
    MotorStateMachine_Run();
    MotorStateMachine_SetHallValid(true);
    MotorStateMachine_Run();
    MotorControl_Run();

    unsigned int disablesBeforeEStop = s_disableCalls;
    MotorControl_EStop(EV_FAULT_DISTANCE_LIMIT);
    MotorStateMachine_Run();
    MotorControl_Run();

    assert(MotorStateMachine_GetState() == EV_MOTOR_STATE_ESTOP_BRAKING);
    assert(MotorStateMachine_GetLatchedFault() == EV_FAULT_DISTANCE_LIMIT);
    assert(s_lastDuty == 0U);
    assert(s_commutationEnabled == 0U);
    assert(s_brakeCalls == 1U);

    s_fakeRpm = 0U;
    MotorControl_Run();
    MotorStateMachine_Run();
    assert(MotorStateMachine_GetState() == EV_MOTOR_STATE_FAULT_LATCHED);
    assert(s_disableCalls == disablesBeforeEStop + 1U);

    MotorStateMachine_AcknowledgeFault();
    MotorStateMachine_Run();
    assert(MotorStateMachine_GetState() == EV_MOTOR_STATE_IDLE);

    return 0;
}

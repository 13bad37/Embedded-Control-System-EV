#pragma once

#include <stdint.h>

typedef enum
{
    EV_SYSTEM_MODE_BOOT = 0,
    EV_SYSTEM_MODE_IDLE,
    EV_SYSTEM_MODE_DRIVE,
    EV_SYSTEM_MODE_FAULT
} EvSystemMode_t;

typedef enum
{
    EV_MOTOR_STATE_IDLE = 0,
    EV_MOTOR_STATE_STARTING,
    EV_MOTOR_STATE_RUNNING,
    EV_MOTOR_STATE_STOPPING,
    EV_MOTOR_STATE_ESTOP_BRAKING,
    EV_MOTOR_STATE_FAULT_LATCHED
} EvMotorState_t;

typedef enum
{
    EV_FAULT_NONE = 0,
    EV_FAULT_STARTUP,
    EV_FAULT_SENSOR,
    EV_FAULT_MOTOR,
    EV_FAULT_POWER_LIMIT,
    EV_FAULT_DISTANCE_LIMIT,
    EV_FAULT_USER_ESTOP,
    EV_FAULT_COMMUNICATION
} EvFaultCode_t;

typedef struct
{
    uint32_t desired_speed_rpm;
    uint32_t reference_speed_rpm;
    uint32_t actual_speed_rpm;
    uint16_t pwm_duty_per_mille;
    EvMotorState_t state;
} EvMotorStatus_t;

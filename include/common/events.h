#pragma once

#include <stdint.h>
#include "common/project_types.h"
#include "common/sensor_types.h"

typedef enum
{
    EV_EVENT_NONE = 0,
    EV_EVENT_MOTOR_START_REQUESTED,
    EV_EVENT_MOTOR_STOP_REQUESTED,
    EV_EVENT_MOTOR_SPEED_REQUESTED,
    EV_EVENT_ESTOP_REQUESTED,
    EV_EVENT_FAULT_ACK_REQUESTED,
    EV_EVENT_SENSOR_RAW_SAMPLE,
    EV_EVENT_SENSOR_FILTERED_SAMPLE,
    EV_EVENT_SENSOR_THRESHOLD_UPDATED,
    EV_EVENT_FAULT_RAISED,
    EV_EVENT_GUI_COMMAND
} EvEventType_t;

typedef struct
{
    EvEventType_t type;
    uint32_t timestamp_ms;
    EvSensorId_t sensor_id;
    uint32_t value;
    EvFaultCode_t fault;
} EvEventMessage_t;

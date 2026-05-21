#pragma once

#include <stdint.h>

void MotorDriver_Init(void);
void MotorDriver_KickstartPlaceholder(void);
void MotorDriver_EnableOutputs(void);
void MotorDriver_DisableOutputs(void);
void MotorDriver_SetPwmDuty(uint16_t duty_per_mille);
void MotorDriver_UpdateCommutationFromHall(uint8_t hall_a, uint8_t hall_b, uint8_t hall_c);
uint32_t MotorDriver_GetHallEdgeCount(void);
uint32_t MotorDriver_GetRpm(void);
void MotorDriver_ResetHallState(void);
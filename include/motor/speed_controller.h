#pragma once

#include <stdint.h>

void SpeedController_Init(void);
uint16_t SpeedController_Update(uint32_t reference_rpm, uint32_t measured_rpm);


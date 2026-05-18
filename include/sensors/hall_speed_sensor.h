#ifndef HALL_SPEED_SENSOR_H
#define HALL_SPEED_SENSOR_H

#include <stdint.h>
#include <stdbool.h>


#define HALL_MOTOR_POLE_PAIRS       7

#define HALL_ZERO_SPEED_TIMEOUT     3

void HallSpeedSensor_Init(void);

int HallSpeedSensor_GetRPM(void);

int HallSpeedSensor_GetRawRPM(void);

void HallA_ISR(void);
void HallB_ISR(void);
void HallC_ISR(void);

void HallSpeedTimer_ISR(void);

#endif // HALL_SPEED_SENSOR_H
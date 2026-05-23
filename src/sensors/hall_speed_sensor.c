// #include <stdint.h>
// #include <stdbool.h>
// #include <string.h>
 
// #include "FreeRTOS.h"
// #include "task.h"
// #include "queue.h"

// #include "motor/motor_driver.h"
// #include "inc/hw_memmap.h"
// #include "inc/hw_ints.h"
// #include "driverlib/sysctl.h"
// #include "driverlib/gpio.h"
// #include "driverlib/timer.h"
// #include "driverlib/interrupt.h"
// #include "driverlib/pin_map.h"
// #include "app/GPIO_Init.h" 


// #include "motor/motor_driver.h"
// #include "sensors/hall_speed_sensor.h"
// #include "sensors/sensor_manager.h"

// uint32_t motor_speed_rpm;

// void HallSpeedTimer_ISR(void)
// {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
 
//     //Compute raw RPM
//     motor_speed_rpm = MotorDriver_GetRpm();
//     SensorMsg_t msg = { .type = SENSOR_MSG_SPEED, .value = (uint32_t)motor_speed_rpm };
//     if (rawdataQueue != NULL)
//     {
//     xQueueSendToBackFromISR(rawdataQueue, &msg, 0);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//     }
// }


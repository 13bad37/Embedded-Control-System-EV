#include <stdint.h>
#include <stdbool.h>
#include <string.h>
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
 
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
 
#include "sensors/hall_speed_sensor.h"
#include "sensors/sensor_manager.h"
#include "app/GPIO_Init.h"

void HallGpioInit(void)
{
    // Enable GPIO peripherals
    SysCtlPeripheralEnable(HALL_A_PERIPH);
    SysCtlPeripheralEnable(HALL_B_PERIPH);
    SysCtlPeripheralEnable(HALL_C_PERIPH);
    while (!SysCtlPeripheralReady(HALL_A_PERIPH));
    while (!SysCtlPeripheralReady(HALL_B_PERIPH));
    while (!SysCtlPeripheralReady(HALL_C_PERIPH));
 
    // Set as inputs
    GPIOPinTypeGPIOInput(HALL_A_BASE, HALL_A_PIN);
    GPIOPinTypeGPIOInput(HALL_B_BASE, HALL_B_PIN);
    GPIOPinTypeGPIOInput(HALL_C_BASE, HALL_C_PIN);
 
    // Both edges — every rising and falling transition counts
    GPIOIntTypeSet(HALL_A_BASE, HALL_A_PIN, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(HALL_B_BASE, HALL_B_PIN, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(HALL_C_BASE, HALL_C_PIN, GPIO_BOTH_EDGES);
 
    // Enable pin interrupts
    GPIOIntEnable(HALL_A_BASE, HALL_A_PIN);
    GPIOIntEnable(HALL_B_BASE, HALL_B_PIN);
    GPIOIntEnable(HALL_C_BASE, HALL_C_PIN);
 
    // Enable port-level interrupts in NVIC
    IntEnable(INT_GPIOM);
    IntEnable(INT_GPIOH);
    IntEnable(INT_GPION);
}
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

// The global semaphore handle
SemaphoreHandle_t I2C0Semaphore;
SemaphoreHandle_t I2C2Semaphore;

void I2C0_Driver_Init(uint32_t sysClockFreq)
{
    I2C0Semaphore = xSemaphoreCreateBinary();
    

    if (I2C0Semaphore == NULL) {
        while(1); 
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C0)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}

    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    

    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    I2CMasterInitExpClk(I2C0_BASE, sysClockFreq, false);
    IntPrioritySet(INT_I2C0, 0xE0);
    I2CMasterIntEnable(I2C0_BASE);
    IntEnable(INT_I2C0);

    IntMasterEnable();
}

void I2C2_Driver_Init(uint32_t sysClockFreq)
{
    I2C2Semaphore = xSemaphoreCreateBinary();
    

    if (I2C2Semaphore == NULL) {
        while(1); 
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}

    GPIOPinConfigure(GPIO_PN4_I2C2SDA);
    GPIOPinConfigure(GPIO_PN5_I2C2SCL);

    

    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);

    I2CMasterInitExpClk(I2C2_BASE, sysClockFreq, false);
    IntPrioritySet(INT_I2C2, 0xE0);
    I2CMasterIntEnable(I2C2_BASE);
    IntEnable(INT_I2C2);

    IntMasterEnable();
}
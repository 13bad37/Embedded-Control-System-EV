#include "motor/motor_driver.h"

#include "common/project_config.h"
#include "motorlib.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "FreeRTOS.h"
#include "task.h"
extern uint32_t g_ui32SysClock;
static void HallSensorHandler(void);

void MotorDriver_Init(void)
{
    UARTprintf("SysClock value: %d\r\n", g_ui32SysClock);
    UARTprintf("PWM period: %d\r\n", PROJECT_MOTORLIB_PWM_PERIOD_US);
    UARTprintf("initMotorLib starting\r\n");
    bool result = initMotorLib(PROJECT_MOTORLIB_PWM_PERIOD_US);
    UARTprintf("initMotorLib returned: %d\r\n", result);
    initMotorLib(PROJECT_MOTORLIB_PWM_PERIOD_US);
    UARTprintf("initMotorLib done\r\n");

    UARTprintf("GPIO peripheral enable starting\r\n");
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    UARTprintf("GPIO peripherals ready\r\n");

    GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_2);
    GPIOPinTypeGPIOInput(GPIO_PORTN_BASE, GPIO_PIN_2);
    UARTprintf("GPIO pins configured\r\n");

    GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_PIN_2, GPIO_BOTH_EDGES);
    UARTprintf("GPIO interrupts configured\r\n");

    GPIOIntRegister(GPIO_PORTM_BASE, HallSensorHandler);
    GPIOIntRegister(GPIO_PORTH_BASE, HallSensorHandler);
    GPIOIntRegister(GPIO_PORTN_BASE, HallSensorHandler);
    UARTprintf("GPIO interrupts registered\r\n");

    GPIOIntEnable(GPIO_PORTM_BASE, GPIO_INT_PIN_3);
    GPIOIntEnable(GPIO_PORTH_BASE, GPIO_INT_PIN_2);
    GPIOIntEnable(GPIO_PORTN_BASE, GPIO_INT_PIN_2);
    UARTprintf("GPIO interrupts enabled\r\n");

    IntPrioritySet(INT_GPIOM, 0xC0);
    IntPrioritySet(INT_GPIOH, 0xC0);
    IntPrioritySet(INT_GPION, 0xC0);
    UARTprintf("Priorities set\r\n");


}

void MotorDriver_KickstartPlaceholder(void)
{
    /*
     * Assignment startup flow needs an open loop kickstart before closed loop.
     * Hook this to MotorLib once state machine logic is being written
     */
}

void MotorDriver_EnableOutputs(void)
{
    /* TODO: call enableMotor() when hardware init is ready. */
}

void MotorDriver_DisableOutputs(void)
{
    /* TODO: ^^^ disableMotor() ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
}

void MotorDriver_SetPwmDuty(uint16_t duty_per_mille)
{
    // (void)duty_per_mille;
    // Scale 0-1000 down to 0-PWM_PERIOD
    uint16_t scaled = (uint16_t)((duty_per_mille * PROJECT_MOTORLIB_PWM_PERIOD_US) / 1000U);
    setDuty(scaled);
 
}

void MotorDriver_UpdateCommutationFromHall(uint8_t hall_a, uint8_t hall_b, uint8_t hall_c)
{
    // (void)hall_a;
    // (void)hall_b;
    // (void)hall_c;
    /* TODO: */
    updateMotor((bool)hall_a, (bool)hall_b, (bool)hall_c);
}

static void HallSensorHandler(void)
{
    bool hall_a = (bool)(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3) & GPIO_PIN_3);
    bool hall_b = (bool)(GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2) & GPIO_PIN_2);
    bool hall_c = (bool)(GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2) & GPIO_PIN_2);

    MotorDriver_UpdateCommutationFromHall(hall_a, hall_b, hall_c);

    // Clear all three ports — any of them could have triggered
    GPIOIntClear(GPIO_PORTM_BASE, GPIO_INT_PIN_3);
    GPIOIntClear(GPIO_PORTH_BASE, GPIO_INT_PIN_2);
    GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_2);
}
#include "motor/motor_driver.h"

#include "common/project_config.h"
#include "motorlib.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "FreeRTOS.h"
#include "task.h"
#include "utils/uartstdio.h"
#include <stdbool.h>
#include "motor/motor_control.h"

extern uint32_t g_ui32SysClock;
static void HallSensorHandler(void);

static volatile uint32_t s_hallEdgeCount     = 0U;
static volatile uint32_t s_lastEdgeTimestamp = 0U;
static volatile uint32_t s_edgePeriodTicks   = 0U;
static volatile uint8_t  s_hallEdgeValid     = 0U;

void MotorDriver_Init(void)
{
    UARTprintf("SysClock value: %d\r\n", g_ui32SysClock);
    UARTprintf("PWM period: %d\r\n", PROJECT_MOTORLIB_PWM_PERIOD_US);
    UARTprintf("initMotorLib starting\r\n");
    bool result = initMotorLib(PROJECT_MOTORLIB_PWM_PERIOD_US);
    UARTprintf("initMotorLib returned: %d\r\n", result);
    /* REMOVED: duplicate initMotorLib call was here */
    UARTprintf("initMotorLib done\r\n");

    /* Hardware timer for hall edge timestamping */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC_UP);
    TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFFFFFF);
    TimerEnable(TIMER1_BASE, TIMER_A);
    UARTprintf("Hall timestamp timer started\r\n");

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
    /* Reset edge count so we're counting fresh edges only */
    s_hallEdgeCount     = 0U;
    s_hallEdgeValid     = 0U;
    s_edgePeriodTicks   = 0U;
    s_lastEdgeTimestamp = 0U;

    /* Wait 200ms — if motor is already spinning edges will accumulate */
    vTaskDelay(pdMS_TO_TICKS(200));

    if (s_hallEdgeCount > 2U)
    {
        UARTprintf("Motor already spinning — skipping kickstart\r\n");
        return;
    }

    uint32_t edgesBefore = s_hallEdgeCount;

    for (uint16_t duty = 5U; duty <= 30U; duty += 2U)
    {
        bool hall_a = (bool)(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3) & GPIO_PIN_3);
        bool hall_b = (bool)(GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2) & GPIO_PIN_2);
        bool hall_c = (bool)(GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2) & GPIO_PIN_2);

        updateMotor(hall_a, hall_b, hall_c);
        setDuty(duty);

        UARTprintf("Kickstart: duty=%d\r\n", duty);
        vTaskDelay(pdMS_TO_TICKS(150));

        if (MotorDriver_GetHallEdgeCount() > edgesBefore + 2U)
        {
            UARTprintf("Motor started at duty=%d\r\n", duty);
            return;
        }
    }

    UARTprintf("WARNING: kickstart complete\r\n");
}

void MotorDriver_EnableOutputs(void)
{
    enableMotor();
}

void MotorDriver_DisableOutputs(void)
{
    disableMotor();
}

void MotorDriver_SetPwmDuty(uint16_t duty_per_mille)
{
    setDuty(duty_per_mille);
}

void MotorDriver_UpdateCommutationFromHall(uint8_t hall_a, uint8_t hall_b, uint8_t hall_c)
{
    updateMotor((bool)hall_a, (bool)hall_b, (bool)hall_c);
}

uint32_t MotorDriver_GetHallEdgeCount(void)
{
    return s_hallEdgeCount;
}

uint32_t MotorDriver_GetRpm(void)
{
    if (!s_hallEdgeValid || s_edgePeriodTicks == 0U)
    {
        return 0U;
    }

    uint32_t now = TimerValueGet(TIMER1_BASE, TIMER_A);
    uint32_t timeSinceLastEdge = now - s_lastEdgeTimestamp;
    if (timeSinceLastEdge > (g_ui32SysClock / 5U))
    {
        return 0U;
    }

    uint32_t rpm = (uint32_t)(((uint64_t)g_ui32SysClock * 60ULL) /
                   ((uint64_t)s_edgePeriodTicks * (PROJECT_MOTOR_POLE_PAIRS * 6U)));
    return rpm;
}

void MotorDriver_ResetHallState(void)
{
    s_hallEdgeCount     = 0U;
    s_hallEdgeValid     = 0U;
    s_edgePeriodTicks   = 0U;
    s_lastEdgeTimestamp = 0U;
}

static void HallSensorHandler(void)
{
    uint32_t now = TimerValueGet(TIMER1_BASE, TIMER_A);

    if (s_hallEdgeValid)
    {
        s_edgePeriodTicks = now - s_lastEdgeTimestamp;
    }
    s_lastEdgeTimestamp = now;
    s_hallEdgeValid = 1U;
    s_hallEdgeCount++;

    /* Only commutate if motor is running or starting —
       prevents MotorLib re-energising phases during braking */
    EvMotorState_t state = MotorControl_GetState();
    if (state == EV_MOTOR_STATE_RUNNING ||
        state == EV_MOTOR_STATE_STARTING)
    {
        bool hall_a = (bool)(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3) & GPIO_PIN_3);
        bool hall_b = (bool)(GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2) & GPIO_PIN_2);
        bool hall_c = (bool)(GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2) & GPIO_PIN_2);
        MotorDriver_UpdateCommutationFromHall(hall_a, hall_b, hall_c);
    }

    GPIOIntClear(GPIO_PORTM_BASE, GPIO_INT_PIN_3);
    GPIOIntClear(GPIO_PORTH_BASE, GPIO_INT_PIN_2);
    GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_2);
}
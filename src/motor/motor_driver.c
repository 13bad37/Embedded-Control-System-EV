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
static volatile uint8_t s_commutationEnabled = 1U;
static uint32_t s_kickstartElapsedMs = 0U;
static uint32_t s_kickstartStepElapsedMs = 0U;
static uint16_t s_kickstartDuty = 0U;
static uint8_t s_kickstartActive = 0U;
static uint8_t s_kickstartTimedOut = 0U;

#define KICKSTART_INITIAL_DUTY       (5U)
#define KICKSTART_MAX_DUTY           (30U)
#define KICKSTART_DUTY_STEP          (2U)
#define KICKSTART_STEP_PERIOD_MS     (150U)
#define KICKSTART_TIMEOUT_MS         (3000U)

void MotorDriver_SetCommutationEnabled(uint8_t enabled)
{
    s_commutationEnabled = enabled;
}

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

static void ApplyKickstartDuty(void)
{
    bool hall_a = (bool)(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3) & GPIO_PIN_3);
    bool hall_b = (bool)(GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2) & GPIO_PIN_2);
    bool hall_c = (bool)(GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2) & GPIO_PIN_2);

    updateMotor(hall_a, hall_b, hall_c);
    setDuty(s_kickstartDuty);
}

void MotorDriver_BeginKickstart(void)
{
    /* Start small and return straight away so E-stop still gets checked. */
    s_hallEdgeCount     = 0U;
    s_hallEdgeValid     = 0U;
    s_edgePeriodTicks   = 0U;
    s_lastEdgeTimestamp = 0U;
    s_kickstartElapsedMs = 0U;
    s_kickstartStepElapsedMs = 0U;
    s_kickstartDuty = KICKSTART_INITIAL_DUTY;
    s_kickstartActive = 1U;
    s_kickstartTimedOut = 0U;
    ApplyKickstartDuty();
}

void MotorDriver_RunKickstart(uint32_t elapsed_ms)
{
    if (!s_kickstartActive)
    {
        return;
    }

    if (s_hallEdgeCount >= HALL_VALID_EDGE_THRESHOLD)
    {
        s_kickstartActive = 0U;
        return;
    }

    s_kickstartElapsedMs += elapsed_ms;
    s_kickstartStepElapsedMs += elapsed_ms;
    if (s_kickstartElapsedMs >= KICKSTART_TIMEOUT_MS)
    {
        s_kickstartActive = 0U;
        s_kickstartTimedOut = 1U;
        UARTprintf("E-STOP: startup could not get hall feedback\r\n");
        return;
    }

    if (s_kickstartStepElapsedMs >= KICKSTART_STEP_PERIOD_MS)
    {
        s_kickstartStepElapsedMs = 0U;
        if (s_kickstartDuty < KICKSTART_MAX_DUTY)
        {
            s_kickstartDuty += KICKSTART_DUTY_STEP;
            if (s_kickstartDuty > KICKSTART_MAX_DUTY)
            {
                s_kickstartDuty = KICKSTART_MAX_DUTY;
            }
            ApplyKickstartDuty();
        }
    }
}

uint8_t MotorDriver_KickstartTimedOut(void)
{
    return s_kickstartTimedOut;
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

    if (s_commutationEnabled)
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

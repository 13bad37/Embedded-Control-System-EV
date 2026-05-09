#include "app/system_init.h"

#include <stdbool.h>
#include <stdint.h>

#include "app/fault_manager.h"
#include "app/integration.h"
#include "app/i2c_driver.h"
#include "common/serial_plotter.h"
#include "common/project_config.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "gui/gui_manager.h"
#include "motor/motor_control.h"
#include "motor/motor_state_machine.h"
#include "sensors/sensor_manager.h"
#include "common/uartstdio.h"


uint32_t g_ui32SysClock = PROJECT_SYSTEM_CLOCK_HZ;

void SystemInit_ConfigureBoard(void)
{
    /*
     * Keep clock setup here so FreeRTOS, DriverLib, and MotorLib work together. 
     * GPIO, ADC, PWM, I2C, UART, and interrupt setup can be added as we do modules
     */
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240),
                                            PROJECT_SYSTEM_CLOCK_HZ);


    I2C0_Driver_Init(g_ui32SysClock);
    I2C2_Driver_Init(g_ui32SysClock);
    UART_Driver_Init(g_ui32SysClock);
}

void AppModules_Init(void)
{
    MotorControl_Init();
    MotorStateMachine_Init();
    SensorManager_Init();
    GuiManager_Init();
    FaultManager_Init();
    Integration_Init();
    SerialPlotter_Init();
}

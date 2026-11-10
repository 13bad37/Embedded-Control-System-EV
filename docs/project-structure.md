# Project structure

`main.c` configures the board, initialises the modules and shared resources, creates the tasks, then starts the scheduler. Control logic stays in the modules below.

| Path | Responsibility |
| --- | --- |
| `src/motor/` | Hall feedback, MotorLib wrapper, startup sequence, PI speed control, ramp limiting and motor states |
| `src/sensors/` | Current acquisition, I2C sensor drivers, sample queue, filters and threshold checks |
| `src/gui/` | Touchscreen dashboard, sensor plots, threshold controls, clock and fault history |
| `src/rtos/` | Task creation, periodic task loops and emergency-stop signalling |
| `src/app/` | Board, UART, I2C and ADC setup; manual fault inputs |
| `src/common/` | Shared events and UART telemetry |
| `include/` | Matching module headers, message types and configuration |
| `src/drivers/` | Kentec display, touchscreen and board support |
| `src/freertos_kernel/` | FreeRTOS kernel and Cortex-M4F port sources compiled into the firmware |
| `src/tivaware_driverlib/` | DriverLib sources compiled with the firmware's floating-point ABI |
| `lib/grlib/` | Graphics library used by the dashboard |
| `lib/motorlib/` | MotorLib header, static archive and supplied API notes |
| `vendor/tivaware/` | TivaWare SDK, including shared headers and FreeRTOS dependencies |
| `scripts/` | Build-date helper and host-test runner |
| `test/` | C regression tests and host substitutes for hardware/RTOS interfaces |

## Startup

```text
main.c
  -> SystemInit_ConfigureBoard()
  -> AppModules_Init()
  -> MessageBus_Init()
  -> RtosTasks_CreateAll()
  -> vTaskStartScheduler()
```

`platformio.ini` selects the board, linker script, include paths and MotorLib archive. It uses the repository-local SDK. `platformio.local.example.ini` shows how to override the TivaWare path through an ignored `platformio.local.ini` when needed.

A few earlier interfaces remain as compatibility hooks. `src/app/integration.c`, the generic ISR functions in `src/app/interrupt_handlers.c` and the `SensorFilter_Update()` wrapper do not implement the active data path. Motor interrupts live in `motor_driver.c`; power is sampled by `SensorTask`; filtering runs through `sensor_manager.c`. The touchscreen driver owns its interrupt path. The dashboard currently contains the plot rendering; `gui_sensor_plot.c` is not a separate plotting implementation.

# Hardware notes

## Current target

- EK-TM4C1294XL LaunchPad, TM4C1294NCPDT at 120 MHz.
- Kentec 320 x 240 SSD2119 touchscreen and its board-support drivers.
- DRV832x driver board, Hall-sensored BLDC motor and MotorLib static library.
- OPT3001 ambient-light sensor, SHT31 temperature/humidity sensor and VL53L0X time-of-flight distance sensor.

The source contains the pin configuration below. This is a firmware pin map, not a complete wiring schematic for the driver board.

| Signal | MCU pin / peripheral | Source |
| --- | --- | --- |
| Hall A, B, C | PM3, PH2, PN2 | `src/motor/motor_driver.c` |
| Sensor SDA / SCL | PN4 / PN5, I2C2 | `src/app/i2c_driver.c` |
| Current sense | PE3 / AIN0 and PD7 / AIN4, ADC1 sequence 2 | `include/app/adc_init.h` |
| Serial RX / TX | PA0 / PA1, UART0 at 115200 baud | `src/app/uart_driver.c` |
| Emergency stop / acknowledge | PJ0 / PJ1, active low with pull-ups | `src/app/fault_manager.c` |
| Day/night LED | PN0 | `src/gui/gui_dashboard.c` |

MotorLib owns the phase-drive pins and uses Timer0 for PWM. Keep those resources free when changing the board configuration. Its API and duty-unit notes are in [the supplied MotorLib readme](../lib/motorlib/README_v1.1.txt).

## Settings to check before running

`include/common/project_config.h` contains the motor's pole-pair count, requested-speed limits, reference ramp rates and task periods. The current defaults are seven pole pairs, 250-1500 RPM and a 50 microsecond PWM period.

`src/motor/speed_controller.c` holds the PI gains, feed-forward expression and duty limits. These values are specific to the present motor setup. The startup sequence has its own duty limits in `src/motor/motor_driver.c`.

The current-to-power conversion uses the supply voltage, shunt resistance, amplifier gain and ADC reference in `include/sensors/motor_power_sensor.h`. Those constants must match the actual circuit; the calculation is not an independent power measurement.

Power sampling uses a processor-triggered ADC1 sequence on a 5 ms task period. The older `power_timer_init.c` helper is not called by the current startup path. The touchscreen has a separate ADC acquisition path.

Start with the motor mechanically secured, the shaft clear and a current-limited supply. Check the manual stop and acknowledgement path before increasing the requested speed. No on-board run is part of the host-test command.

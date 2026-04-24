# Assignment Requirements
## Motor Control
The assignemnt says we are using a BLDC motor controller with a MotorLib/DRV API, state machine, closed loop speed control, ramp limits, e-stop handling and UART serial plotting. 


Current placeholders:

- `motor_control.c` owns the public motor API shape: init, start, stop, set speed, e-stop, get speed/state/status
- `motor_state_machine.c` owns the states: Idle, Starting, Running, E-Stop Braking, Fault Latched.
- `motor_driver.c` is the future wrapper around MotorLib and the DRV832x breakout board.
- `ramp_controller.c` is where the normal ramp and e-stop ramp logic should go
- `speed_controller.c` is where the PI and closed loop speed controller should go later.

MotorLib is stored in `lib/motorlib`. Look at `motorlib-integration.md` for the exact API and build notes.

## Sensing

Flow works like this:

```text
Sensor Data -> ISR Capture -> Buffering/Queue -> Filtering Task -> Application Use
```

Placeholders:

- `sensor_manager.c` is the RTOS task integration point 
- `sensor_filter.c` is where filtering should happen later. DO NOT FILTER IN ISRs
- `sensor_types.h` defines raw and filtered data and all the threshold structs
- `interrupt_handlers.c` has ISR placeholders for hall edges, ADC sampling, timer sampling, and touchscreen handling.

Required sensors:

- Motor power sensing: `motor_power_sensor.c`
- Motor speed using hall sensors: `hall_speed_sensor.c`
- OPT3001 ambient light: `ambient_light_sensor.c`
- SHT31 temperature/humidity: `sht31_sensor.c`
- VL53L0X distance: `vl53l0x_sensor.c`

Task sheet says motor power and distance thresholds have to be able to trigger e-stop events. Those hooks are represented by `EvSensorThresholds_t`, `EvEventMessage_t`, and `FaultManager_RecordFault()`.

## GUI

Task sheet wants motor controls, status display, speed input, clock, threshold configuration, e-stop acknowledgement, day/night indication, optional sensor display, and a second page/tab for sensor plots.

Current placeholders:

- `gui_manager.c` coordinates GUI updates.
- `gui_dashboard.c` is the future main control and status page.
- `gui_sensor_plot.c` is the future sensor plot page/tab.

## RTOS And Communication
Current placeholders:

- `task_manager.c` creates Motor, Sensor, GUI, Fault, Integration, and SerialPlot tasks.
- `message_bus.c` creates the shared event queue.
- `events.h` defines event types for motor requests, e-stop, fault acknowledgement, sensor samples, threshold updates, and faults.
- `serial_plotter.c` is the UART/SerialPlot placeholder for motor and sensor debug frames.

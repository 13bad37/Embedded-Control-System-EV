# Project Structure

```text
.
|-- include/
|   |-- app/
|   |-- common/
|   |-- gui/
|   |-- motor/
|   |-- rtos/
|   `-- sensors/
|-- src/
|   |-- app/
|   |-- common/
|   |-- freertos_kernel/
|   |-- gui/
|   |-- motor/
|   |-- rtos/
|   |-- sensors/
|   |-- tivaware_driverlib/
|   |-- platformio_linker.ld
|   |-- startup_gcc.c
|   `-- main.c
|-- lib/
|-- vendor/
|   `-- tivaware/
|-- docs/
|-- test/
|-- platformio.ini
|-- platformio.local.example.ini
`-- README.md
```

## Folders

`include/` is for all the public headers for each module. If another module needs to call your code, put the declaration here.

`src/` is for the matching implementation files. Most of the assignment code should be here instead of going into `main.c`.

`src/rtos/` is the FreeRTOS task creation, placeholder task loops and shared RTOS resources(queues, etc), this is gonna be the integration layer for now.

`src/freertos_kernel/` has the FreeRTOS source files that need to be compiled for te project. I just copied them from my local FreeRTOS directory

`src/tivaware_driverlib/` has the small subset of TivaWare DriverLib source files that the board clock and Motorlib need. Had to do compile it from source to match the assignment's `softfp` build flags.

`src/common/` and `include/common/` just store al the shared types, constants, event/message definitions, sensor data structs and UART serial plotting hooks

`src/motor/` is for motor control, the motor state machine, MotorLib/DRV832x wrapper hooks, ramp limiting, and the closed loop speed controller placeholder 

`src/sensors/` is for the sensor manager, filter placeholder, and the five sensor placeholders we actually need: motor power, hall speed, OPT3001 ambient light, SHT31 temperature/humidity, and VL53L0X distance. We have to wire it up properly once all the sensor code is done.

`src/gui/` is for the GUI/display/input layer. It's got a dashboard placeholder for status/controls/thresholds and a sensor plot placeholder for the second page

`src/app/` is for all the high level stuff like the board setup, fault handling, integration behaviour and the ISR placeholders

`lib/` is only for small libraries if we need them, copied from online

`lib/motorlib/` is just stuff from the MotorLib file we were given, its just the header and static library

`vendor/tivaware/` is the copied TivaWare install which includes the FreeRTOS headers and portable files. I did this so we don't have to keep editing the main platform.ini file based on our local directories for wherever FreeRTOS and the TivaWare kernal are installed

`test/` is for host side or PlatformIO tests, was told to add this by previous students of the subject since we have to discus testing methods in our report, so may as well add some dedicated tests here later on. 

`platformio.local.example.ini` shows how to override local paths without editing the PlatformIO file we all share. Just copy it to `platformio.local.ini` only if you can't use the repo local `vendor/tivaware` layout.

`docs/assignment-requirements-map.md` this just contexulaises the assigment requirements to all the placeholders in the codebase right now to make it easier. l
## Main Flow
Basically, I made it so that everything flows like:

```text
main.c
  -> SystemInit_ConfigureBoard()
  -> AppModules_Init()
  -> MessageBus_Init()
  -> RtosTasks_CreateAll()
  -> vTaskStartScheduler()
```
Make sure main.c doesn't become a huge file.
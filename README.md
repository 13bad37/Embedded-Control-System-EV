# EGH456 EV Motor Control System
## Architecture

`main.c` stays high level: configure the board, initialise all modules, create RTOS resources and tasks, then start the FreeRTOS scheduler. Actual code logic lives in its own module folders under `src/` and `include/`, while RTOS setup and shared message definitions are kept separate so integration work doesn't leak into every module

## Current Status
- PlatformIO project file is in place
- FreeRTOS config, kernel source files, and task creation scaffolding are done
- TivaWare has been copied into `vendor/tivaware` so the shared build doesn't depend on any local paths.
- Copied the MotorLib provided in assigbment spec into `lib/motorlib` and linked, but the motor wrapper is still just placeholder stuff
- Motor, GUI, fault&safety, integration, and shared event modules are placeholder stubs
- Sensor stubs are based on the sensors we chose + have top use: motor power, hall speed, ambient light, SHT31 temperature/humidity, and VL53L0X distance.

See [docs/project-structure.md](docs/project-structure.md) for the folder map and what each part is meant to own

See [docs/assignment-requirements-map.md](docs/assignment-requirements-map.md) for how the task sheet brief applys to the current modules and stubs


## Expected Workflow
1. Make sure we keep module specific code inside the matching `src/<module>/` and `include/<module>/` folders.
2. Keep `main.c` high level.
3. Only add actual hardware setup, drivers, queues and control when the module is ready.
4. Would be really nice if we could update the docs when a placeholder becomes actual code

## Build Notes

```sh
pio run
```

The shared config uses the vendored TivaWare tree under `vendor/tivaware`, so we can build from the same repo without editing `platformio.ini`.

If someone really needs to use an external TivaWare install, copy `platformio.local.example.ini` to `platformio.local.ini` and override `tivaware` there. That local file is ignored by git.


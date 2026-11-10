# Libraries

`grlib/` contains the graphics library used by the Kentec dashboard. PlatformIO compiles it as a private library.

`motorlib/` contains the supplied `motorlib.h` interface and `libmotorlib.a` archive. It handles the low-level phase drive, enable/disable and brake commands. [Its API notes](motorlib/README_v1.1.txt) describe reserved pins, Timer0 use and PWM duty units. The application supplies Hall feedback, speed regulation and the control state machine.

The full TivaWare SDK is under `vendor/tivaware/`, outside this directory, so PlatformIO does not try to compile every SDK example as a private library. A selected DriverLib subset is compiled from `src/tivaware_driverlib/` with the same `softfp` ABI as the firmware.

Bundled third-party source, headers and archives retain their original attribution and licence notices. The application does not replace those notices with a blanket licence.

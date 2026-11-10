# Host regression tests

Run from the repository root with Python 3 and a C compiler on `PATH`:

```sh
python3 scripts/test_host.py
```

The runner builds each test in a temporary directory with C11, `-Wall`, `-Wextra` and `-Werror`, runs it, then removes the binaries. Set `CC` to select another compiler:

```sh
CC=clang python3 scripts/test_host.py
```

| Test | Checks |
| --- | --- |
| `aest_clock_test.c` | AM/PM formatting, elapsed time, year rollover and leap day |
| `fault_history_test.c` | One record per fault episode, newest-first ordering and bounded history |
| `motor_estop_test.c` | Normal stop, zero forward duty during emergency braking, disabled commutation, fault latching and acknowledgement |
| `vl53l0x_init_test.c` | Rejection of an unexpected device ID and the expected initialisation/calibration register writes |

`host_stubs/` substitutes only the RTOS and hardware-facing dependencies needed by these tests. The runner searches `include/` first so the tests use the firmware's actual project constants.

These tests execute real clock, history, motor-control and distance-driver code against mocked inputs. They do not check sensor wiring, motor tuning, braking distance, interrupt timing or touchscreen behaviour. Those require the connected board.

The firmware build is a separate check:

```sh
pio run
```

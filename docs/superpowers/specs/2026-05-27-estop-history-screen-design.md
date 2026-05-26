# Emergency Stop History Screen Design

## Goal

Add one GUI page that displays recent emergency-stop events with timestamps,
without changing motor control, distance-sensor operation, fault-latch
acknowledgement, or the existing fault popup.

## Scope

The history is session-only. Records are kept in RAM and reset when the board
is reset or loses power. The firmware has a UI runtime clock but no existing
nonvolatile fault log or validated real-time-clock persistence mechanism, so
flash persistence is outside this feature.

## User Interface

- Add a fourth dashboard panel labelled `Faults`, reached through the existing
  previous and next panel buttons after `Plots`.
- Keep the existing banner, navigation buttons, and bottom date/time widget.
- Display the newest emergency stop first in a compact list.
- Each record contains the current displayed date/time and its fault cause:
  `Distance`, `Power`, `Button`, `Start`, `Sensors`, `Motor`, or `Comms`.
- When no fault has occurred during the current session, display a single
  empty-state line.

The existing emergency-stop popup message and acknowledgement button remain
unchanged and still take priority when a fault is latched.

## Data Flow

The dashboard already observes `MotorStateMachine_GetLatchedFault()` while it
updates the fault UI. It will append one history entry only when the displayed
motor state first transitions into an emergency-stop/fault-latched sequence
with a non-`EV_FAULT_NONE` latched cause. Remaining in the fault state, changing
panels, or repainting widgets must not append duplicates.

A fixed-size ring/list in GUI memory stores recent records. Once full, adding a
new record discards the oldest record; no heap allocation, queues, flash
writes, or motor-task work are added.

## Isolation

- No modification to the E-stop trigger, motor state machine, sensor manager,
  distance threshold, or acknowledgement behavior.
- All LCD drawing continues in the display task; button callbacks only request
  existing deferred UI work.
- History rendering is performed only when the `Faults` page is visible or is
  opened, preventing new drawing contention.

## Validation

- Add host-testable history storage/formatting coverage for inserting a fault,
  avoiding duplicate entries, retaining newest-first order, and dropping the
  oldest entry when capacity is exceeded.
- Re-run existing motor emergency-stop/normal-stop and VL53L0X tests.
- Run `pio run` to ensure the target firmware builds.
- On hardware, trigger a distance fault and a manual E-stop, acknowledge each,
  then confirm one timestamped row appears per stop and all existing popup and
  motor controls behave as before.

#!/usr/bin/env python3
"""Build and run the C regression tests without a connected board."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
TESTS = (
    ("aest_clock", ("test/aest_clock_test.c", "src/gui/aest_clock.c")),
    ("fault_history", ("test/fault_history_test.c", "src/gui/fault_history.c")),
    ("motor_estop", ("test/motor_estop_test.c",)),
    ("vl53l0x_init", ("test/vl53l0x_init_test.c",)),
)


def main():
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror",
             "-Iinclude", "-Itest/host_stubs", "-Ilib/motorlib"]
    with tempfile.TemporaryDirectory(prefix="ev-host-tests-") as directory:
        for name, sources in TESTS:
            binary = Path(directory) / (name + (".exe" if os.name == "nt" else ""))
            subprocess.run([*compiler, *flags, *sources, "-o", str(binary)],
                           cwd=ROOT, check=True)
            subprocess.run([str(binary)], cwd=ROOT, check=True)
            print(f"PASS {name}", flush=True)
    print(f"{len(TESTS)} host test suites passed.")


if __name__ == "__main__":
    main()

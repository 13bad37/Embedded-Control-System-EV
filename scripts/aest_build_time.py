import os
import time

Import("env")

# GCC expands __DATE__/__TIME__ using the process TZ to keep the display on AEST.
os.environ["TZ"] = "Australia/Brisbane"
if hasattr(time, "tzset"):
    time.tzset()
env["ENV"]["TZ"] = "Australia/Brisbane"

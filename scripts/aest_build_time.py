import os
import time

Import("env")

# Keep the build date current for the display; the demo start time itself is
# now hardcoded in the dashboard.
os.environ["TZ"] = "Australia/Brisbane"
if hasattr(time, "tzset"):
    time.tzset()
env["ENV"]["TZ"] = "Australia/Brisbane"

generated_dir = os.path.join(env.subst("$BUILD_DIR"), "generated")
timestamp_stamp = os.path.join(generated_dir, "aest_build_time.stamp")
now_date = time.strftime("%b %d %Y")
now_time = time.strftime("%H:%M:%S")

os.makedirs(generated_dir, exist_ok=True)
with open(timestamp_stamp, "w") as stamp_file:
    stamp_file.write("{} {}\n".format(now_date, now_time))

# Dashboard still uses __DATE__; this dependency makes that date fresh now.
dashboard_object = os.path.join(env.subst("$BUILD_DIR"), "src", "gui", "gui_dashboard.o")
env.Depends(dashboard_object, timestamp_stamp)

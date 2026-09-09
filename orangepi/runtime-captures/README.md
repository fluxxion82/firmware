# Orange Pi Runtime Patch Captures

These files are exact snapshots pulled from the working Orange Pi Zero 3
(`sterling@192.168.6.210`) on 2026-03-05 PST.

They capture **Pi-local runtime patches that are not in the meshtastic firmware
source tree** and would otherwise be lost.

## Captured Files

- `LinuxGPIOPin.cpp.patched`
  - Source on Pi:
    - `~/.platformio/packages/framework-portduino/cores/portduino/linux/gpio/LinuxGPIOPin.cpp`
  - Why it matters:
    - Contains libgpiod v2 safety/retry/null-guard behavior that reduced GPIO
      assertion/abort issues during startup.

- `SX127x.cpp.patched`
  - Source on Pi:
    - `~/firmware/.pio/libdeps/native/RadioLib/src/modules/SX127x/SX127x.cpp`
  - Why it matters:
    - Contains additional Orange Pi troubleshooting instrumentation and
      tolerance for a Portduino-specific `invertIQ(false)` write failure path.

## Important

- These are snapshots of generated/dependency files, not first-party firmware
  sources.
- They are preserved here for reproducibility and forensic diffing.
- If you need to re-apply these in a new environment, compare these captures to
  the newly downloaded PlatformIO/package versions and patch accordingly.

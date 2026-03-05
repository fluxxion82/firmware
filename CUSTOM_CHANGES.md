# Meshtasticd Custom Changes for Orange Pi Zero 3

## Overview

This document tracks all custom changes made to the meshtasticd firmware source
to get it working on the Orange Pi Zero 3 with RFM95W LoRa module.

## Build Environment

- **Base Commit**: `15297cb` on `develop` branch
- **Platform**: Orange Pi Zero 3 (ARM64, Allwinner H618)
- **LoRa Module**: RFM95W (SX1276 compatible)
- **SPI Device**: /dev/spidev1.1
- **GPIO Chip**: gpiochip1
  - IRQ Pin: GPIO 70 (DIO0)
  - Reset Pin: GPIO 71

## Git-Tracked Changes

### 1. src/mesh/RadioLibInterface.cpp

**Purpose**: Debug logging for SPI transfers

```diff
+#include <cstdio>
 #include "RadioLibInterface.h"
...
 #if ARCH_PORTDUINO
 void LockingArduinoHal::spiTransfer(uint8_t *out, size_t len, uint8_t *in)
 {
+    fprintf(stderr, "LOCKING spiTransfer: len=%zu out[0]=0x%02x\n", len, out[0]);
+    fflush(stderr);
     spi->transfer(out, in, len);
+    fprintf(stderr, "LOCKING result: in[0]=0x%02x in[1]=0x%02x\n", in[0], len > 1 ? in[1] : 0);
+    fflush(stderr);
 }
 #endif
```

## Non-Git-Tracked Changes (.pio/libdeps)

These changes are in downloaded dependencies and will be lost if dependencies are re-downloaded.

### 2. .pio/libdeps/native/RadioLib/src/Module.cpp

**Purpose**: Debug logging for SPI register operations

Added fprintf statements in:
- `SPIsetRegValue()` - logs register writes with address, value, bit ranges
- `SPIreadRegister()` - logs register reads
- `SPIwriteRegister()` - logs register writes

### 3. .pio/libdeps/native/RadioLib/src/modules/SX127x/SX127x.cpp

**Purpose**: Debug logging for SX127x initialization sequence

Added fprintf statements in `SX127x::begin()` to trace:
- findChip() calls and results
- standby() calls and results
- config() calls and results
- Modem mode checks
- setSyncWord, setCurrentLimit, setPreambleLength, invertIQ results

## Known Issues

### Issue 1: RF95 init result -20 (RADIOLIB_ERR_WRONG_MODEM)

**Symptoms**:
- `RF95 init result -20` in logs
- Followed by `free(): invalid pointer` crash

**Root Cause Analysis**:
The -20 error (RADIOLIB_ERR_WRONG_MODEM) occurs when one of the configuration
functions (setGain, setFrequency, etc.) calls getActiveModem() and the returned
value doesn't match expected LoRa mode constant.

Possible causes:
1. SPI timing issues causing register reads to return wrong values
2. Radio state becoming inconsistent during configuration

**Workaround**:
- Reduce SPI speed from 500kHz to 100kHz
- Ensure proper reset timing (2s low, 3s high recovery)

### Issue 2: IRQ Flood (0x40 RxDone)

**Symptoms**:
- `setIrqFlags: irq=0x00000040` repeated 40+ times in logs
- Leads to crash after TX

**Analysis**:
The RxDone interrupt (bit 6) keeps firing in a tight loop. This may be caused by:
1. Interrupt not being cleared properly
2. Radio stuck in a bad state after TX

### Issue 3: free(): invalid pointer crash

**Symptoms**:
- Crash with "free(): invalid pointer" after RF95 init fails
- Happens when `delete rIf` is called in RadioInterface.cpp

**Analysis**:
When RF95Interface::init() returns false, the RF95Interface destructor is called.
The destructor attempts to free memory that wasn't properly allocated because
init() failed partway through.

**Fix Required**:
Proper null checks and initialization guards in RF95Interface destructor.

## Configuration Files

### /etc/meshtasticd/config.d/lora-rfm95w-opi3.yaml

```yaml
Lora:
  Module: RF95
  spidev: spidev1.1
  spiSpeed: 500000  # Consider 100000 for stability
  gpiochip: 1
  IRQ:
    pin: 70
    gpiochip: 1
    line: 70
  Reset:
    pin: 71
    gpiochip: 1
    line: 71
```

### /usr/local/bin/reset-lora.sh

```bash
#!/bin/bash
GPIO=71
CHIP=/dev/gpiochip1
gpioset $CHIP $GPIO=0 2>/dev/null || true
sleep 1
gpioset $CHIP $GPIO=1 2>/dev/null || true
sleep 2
echo "LoRa reset complete"
```

Note: gpioset v2.x syntax is: `gpioset -c gpiochip1 71=0`

## Build Instructions

```bash
cd /home/sterling/firmware
source /home/sterling/meshtastic-venv/bin/activate
pio run -e native
# Binary output: .pio/build/native/program
```

## Installation

```bash
sudo systemctl stop meshtasticd
sudo cp .pio/build/native/program /usr/bin/meshtasticd
sudo systemctl start meshtasticd
```

## Debugging Tips

1. Run manually with filtered output:
   ```bash
   sudo /usr/bin/meshtasticd 2>&1 | grep -v 'LOCKING\|SPIread\|SPIwrite\|SPIset\|spiTransfer'
   ```

2. Check GPIO state:
   ```bash
   gpioinfo | grep -E 'line 70|line 71'
   ```

3. Monitor kernel messages:
   ```bash
   dmesg -w | grep gpio
   ```

4. Check service logs:
   ```bash
   journalctl -u meshtasticd -f
   ```

## Version Info

- meshtasticd package: 2.7.15.48
- Firmware source: 2.7.20.15297cb (develop branch)
- RadioLib: bundled with firmware
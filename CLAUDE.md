# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BL Led Controller (BLLC) is an ESP32-based firmware that connects to Bambu Lab 3D printers (X1, X1C, P1P, P1S) via MQTT and controls an external LED strip based on printer state. The device provides visual feedback for printing stages, errors, and HMS (Health Management System) alerts.

## Build Commands

This is a PlatformIO project. All commands should be run from the project root.

```bash
# Build for ESP32 (default board)
pio run -e esp32dev

# Build for ESP32-S3
pio run -e esp32s3dev

# Upload firmware to device
pio run -e esp32dev -t upload

# Upload filesystem (LittleFS)
pio run -e esp32dev -t uploadfs

# Clean build
pio run -e esp32dev -t clean

# Monitor serial output
pio device monitor -b 115200
```

## Build Outputs

After building, firmware files are generated in `.firmware/`:
- `BLLC_V{version}.bin` - Merged firmware for initial flashing
- `BLLC_V{version}.bin.ota` - OTA update file

## Architecture

### Core Components (src/blled/)

- **main.cpp** - Entry point; initializes LEDs, filesystem, WiFi, webserver, and MQTT; main loop handles WiFi reconnection and printer discovery
- **types.h** - Global state structures: `PrinterVariables` (printer state), `PrinterConfig` (LED settings), `GlobalVariables` (WiFi credentials)
- **mqttmanager.h** - MQTT client connecting to Bambu printer on port 8883 (TLS); parses printer status, gcode state, HMS errors, and door sensor; runs as FreeRTOS task
- **web-server.h** - AsyncWebServer on port 80; serves compressed HTML from PROGMEM; handles configuration, OTA updates, WiFi setup, and WebSocket status updates
- **leds.h** - PWM LED control using 5 channels (R, G, B, warm white, cold white); `tweenToColor()` for smooth transitions; `updateleds()` state machine for LED behavior
- **filesystem.h** - LittleFS configuration persistence (`/blledconfig.json`)
- **wifi-manager.h** - WiFi connection management; AP mode fallback for initial setup
- **bblPrinterDiscovery.h** - SSDP-based printer discovery on local network

### Web Assets (src/www/)

HTML/CSS/JS files are compressed at build time by `pre_build.py` into `www.h` as gzip-compressed byte arrays stored in PROGMEM.

### Key Data Flow

1. MQTT receives printer status JSON from Bambu printer
2. `ParseCallback()` extracts: gcode_state, stg_cur (stage), hms (errors), lights_report, home_flag (door)
3. State changes trigger `updateleds()` which determines LED color based on priority:
   - Maintenance/Test/Disco modes (override all)
   - Error states (HMS fatal/serious, temperature failures)
   - Print states (pause, stages 0-35)
   - Finish indication
   - LED replication (mirror chamber light)
   - Inactivity timeout

### LED Pin Assignments (ESP32)

| Color | GPIO | PWM Channel |
|-------|------|-------------|
| Red   | 17   | 0           |
| Green | 16   | 1           |
| Blue  | 18   | 2           |
| Warm  | 15   | 3           |
| Cold  | 11   | 4           |

### Printer Stages

Key stage values from MQTT `stg_cur`:
- -1/255: IDLE
- 0: Printing
- 1: Bed leveling
- 2: Preheating
- 8: Calibrating extrusion
- 9: Scanning bed
- 10: First layer inspection
- 14: Cleaning nozzle
- 16/30: Paused

## Configuration

Device stores config in LittleFS at `/blledconfig.json`. Key settings include:
- WiFi credentials (SSID, password)
- Printer connection (IP, serial number, access code)
- LED behavior modes and custom colors
- HMS ignore list for suppressing specific error codes

## Web Interface Endpoints

- `/` - Main setup page (LED configuration)
- `/wifi` - WiFi and printer setup
- `/fwupdate` - OTA firmware upload
- `/backuprestore` - Config backup/restore
- `/webserial` - Live debug log viewer
- `/factoryreset` - Wipe all settings

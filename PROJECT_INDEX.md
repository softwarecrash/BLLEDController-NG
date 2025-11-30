# Project Index: BLLEDController-NG

Generated: 2025-11-30
Version: 2.3.0.argb.1
Codename: Balder

## Overview

ESP32 firmware that connects to Bambu Lab 3D printers (X1, X1C, P1P, P1S) via MQTT and controls addressable RGB LED strips based on printer state using FastLED.

## Project Structure

```
BLLEDController-NG/
├── src/
│   ├── main.cpp              # Entry point: setup(), loop()
│   ├── blled/                # Core firmware modules
│   │   ├── types.h           # Global data structures
│   │   ├── leds.h            # LED control (FastLED)
│   │   ├── patterns.h        # LED patterns (solid, breathing, chase, rainbow, progress)
│   │   ├── mqttmanager.h     # MQTT client for Bambu printer
│   │   ├── mqttparsingutility.h # MQTT JSON parsing
│   │   ├── web-server.h      # AsyncWebServer + WebSocket
│   │   ├── filesystem.h      # LittleFS config persistence
│   │   ├── wifi-manager.h    # WiFi connection + AP mode
│   │   ├── bblPrinterDiscovery.h # SSDP printer discovery
│   │   ├── ssdp.h            # SSDP protocol
│   │   ├── serialmanager.h   # Serial communication
│   │   ├── logSerial.h       # Debug logging
│   │   └── AutoGrowBufferStream.h # Buffer utilities
│   └── www/                  # Web interface assets
│       ├── setupPage.html    # Main LED configuration
│       ├── wifiSetup.html    # WiFi/printer setup
│       ├── updatePage.html   # OTA firmware update
│       ├── backupRestore.html # Config backup/restore
│       ├── webSerialPage.html # Debug log viewer
│       ├── style.css         # Shared styles
│       ├── particleCanvas.js # UI effects
│       ├── blled.svg         # Logo
│       ├── favicon.png       # Icon
│       └── www.h             # Compressed assets (generated)
├── docs/
│   └── manual.md             # User manual
├── platformio.ini            # PlatformIO configuration
├── pre_build.py              # HTML compression script
├── merge_firmware.py         # Firmware merger
└── bblp_sim.py               # Printer simulator (testing)
```

## Core Data Structures (`src/blled/types.h`)

| Structure | Purpose |
|-----------|---------|
| `PrinterVariables` | Printer state: gcodeState, stage, HMS errors, door status |
| `PrinterConfig` | Settings: IP, colors, patterns, timeouts, relay config |
| `GlobalVariables` | WiFi credentials, firmware version |
| `LedConfig` | LED strip: chipType, colorOrder, ledCount, pins |
| `COLOR` | RGB color with hex representation |

### LED Chip Types
`WS2812B`, `SK6812`, `SK6812_RGBW`, `APA102`, `WS2811`, `NEOPIXEL`

### Color Orders
`GRB`, `RGB`, `BRG`, `RBG`, `BGR`, `GBR`, `GRBW`, `RGBW`

### LED Patterns
`SOLID`, `BREATHING`, `CHASE`, `RAINBOW`, `PROGRESS`

## Key Functions

### Entry Points (`src/main.cpp`)
- `setup()` - Initializes LEDs, filesystem, WiFi, webserver, MQTT
- `loop()` - Handles WiFi reconnection, printer discovery

### LED Control (`src/blled/leds.h`)
- `setupLeds()` - Initialize FastLED with configured chip/order
- `updateleds()` - State machine for LED behavior
- `setLedColor()` - Set color with pattern
- `setLedState()` - Control LED on/off
- `setRelayState()` - Control external relay

### Patterns (`src/blled/patterns.h`)
- `applyPattern()` - Main pattern dispatcher
- `applySolidPattern()` - Static color
- `applyBreathingPattern()` - Fade in/out
- `applyChasePattern()` - Running light
- `applyRainbowPattern()` - Color cycle
- `applyProgressPattern()` - Print progress bar
- `runTestSequence()` - LED test mode

### MQTT (`src/blled/mqttmanager.h`)
- `setupMqtt()` - Initialize MQTT client
- `connectMqtt()` - Connect to printer (TLS port 8883)
- `mqttTask()` - FreeRTOS task for MQTT
- `ParseCallback()` - Parse printer JSON status
- `mqttloop()` - MQTT maintenance loop

### Web Server (`src/blled/web-server.h`)
- `setupWebserver()` - Initialize AsyncWebServer
- `handleSetup()` - Serve main config page
- `handleSubmitConfig()` - Save LED configuration
- `handleWiFiSetupPage()` - WiFi setup page
- `handleSubmitWiFi()` - Save WiFi/printer config
- `handleLedTest()` - Trigger LED test
- `websocketLoop()` - Push status updates

### Filesystem (`src/blled/filesystem.h`)
- `setupFileSystem()` - Initialize LittleFS
- `loadFileSystem()` - Load config from `/blledconfig.json`
- `saveFileSystem()` - Persist configuration
- `deleteFileSystem()` - Factory reset

### WiFi (`src/blled/wifi-manager.h`)
- `connectToWifi()` - Connect with stored credentials
- `startAPMode()` - Fallback AP for setup
- `scanNetwork()` - WiFi network scan

### Printer Discovery (`src/blled/bblPrinterDiscovery.h`)
- `bblSearchPrinters()` - SSDP discovery
- `bblIsPrinterKnown()` - Check known printers

## Web Endpoints

| Endpoint | Handler | Description |
|----------|---------|-------------|
| `/` | `handleSetup` | Main LED setup page |
| `/wifi` | `handleWiFiSetupPage` | WiFi/printer setup |
| `/fwupdate` | `handleUpdatePage` | OTA firmware upload |
| `/backuprestore` | `handleConfigPage` | Config backup/restore |
| `/webserial` | `handleWebSerialPage` | Debug log viewer |
| `/submitconfig` | `handleSubmitConfig` | POST LED config |
| `/submitwifi` | `handleSubmitWiFi` | POST WiFi config |
| `/config` | `handleGetConfig` | GET current config |
| `/printerconfig` | `handlePrinterConfigJson` | GET printer config |
| `/ledtest` | `handleLedTest` | Trigger LED test |
| `/wifiscan` | `handleWiFiScan` | Scan WiFi networks |
| `/printerlist` | `handlePrinterList` | List discovered printers |
| `/factoryreset` | `handleFactoryReset` | Wipe all settings |

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| FastLED | ^3.7.7 | Addressable LED control |
| ArduinoJson | 7.4.2 | JSON parsing |
| PubSubClient | 2.8.0 | MQTT client |
| ESP32SSDP | 1.2.1 | SSDP discovery |
| AsyncTCP | latest | Async TCP |
| ESPAsyncWebServer | latest | Web server |
| MycilaWebSerial | ^8.1.1 | Debug serial over web |

## Build Commands

```bash
# Build ESP32 firmware
uv run pio run -e esp32dev

# Upload firmware
uv run pio run -e esp32dev -t upload

# Upload filesystem
uv run pio run -e esp32dev -t uploadfs

# Monitor serial
uv run pio device monitor -b 115200

# Clean build
uv run pio run -e esp32dev -t clean
```

## Build Outputs

Location: `.firmware/`
- `BLLC_V{version}.bin` - Merged firmware for initial flash
- `BLLC_V{version}.bin.ota` - OTA update file

## Configuration

File: `/blledconfig.json` (LittleFS)

Key settings:
- WiFi: SSID, password, BSSID
- Printer: IP, serial number, access code
- LED: chip type, color order, count, data pin, clock pin
- Colors: per-state RGB values with patterns
- Behavior: finish timeout, inactivity timeout, relay config

## Printer Stages (MQTT `stg_cur`)

| Value | Stage |
|-------|-------|
| -1/255 | IDLE |
| 0 | Printing |
| 1 | Bed leveling |
| 2 | Preheating |
| 8 | Calibrating extrusion |
| 9 | Scanning bed |
| 10 | First layer inspection |
| 14 | Cleaning nozzle |
| 16/30 | Paused |

## LED Priority (highest to lowest)

1. Maintenance/Test/Disco modes
2. Error states (HMS fatal/serious, temperature failures)
3. Print states (pause, stages 0-35)
4. Finish indication
5. LED replication (mirror chamber light)
6. Inactivity timeout

## Testing

```bash
# Run printer simulator
python bblp_sim.py
```

## Session Memories

Previous work sessions documented in `.serena/memories/`:
- `session-improvements-2025-11` - November improvements
- `session-relay-feature-2025-11` - Relay feature implementation
- `session-rgbw-fix-2025-11` - RGBW color order fix

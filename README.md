# Bambu Lab FastLED Controller v3.0

The Bambu Lab FastLED Controller (BLFastLED Controller) project is a fork of [BLLEDController-NG](https://github.com/softwarecrash/BLLEDController-NG) by [softwarecrash](https://github.com/softwarecrash) which is a fork of [BL Led Controller](https://github.com/DutchDevelop/BLLEDController) by [DutchDeveloper](https://dutchdevelop.com/).

The significant difference between this project and its upstream sources is support for addressable RGB LED strips instead of RGB-CCT PWM LED strips.

## Changes in v3.0 (FastLED Migration)

This release represents a complete rewrite of the LED control system, switching from PWM-based RGB-CTT strips to addressable LED strips using the FastLED library.

> **IMPORTANT**
>
> Do not upgrade from one of the upstream forks to this project. You _must_ erase your ESP32 before flashing this firmware.


### Supported LED Chipsets

- **WS2812B** (default)
- **SK6812** (RGB and RGBW variants)
- **WS2811**
- **WS2814** (RGBW)
- **APA102** (requires clock pin)
- **NEOPIXEL**

### New Features

#### Pattern Effects
Each printer state can now use animated patterns instead of static colors:
- **Solid** - All LEDs same color (default for most states)
- **Breathing** - Brightness pulsing effect (used for finish, pause, and error states)
- **Chase** - Moving light effect (used for first layer inspection)
- **Rainbow** - Full color cycle (disco mode)
- **Progress Bar** - Visual print progress indicator

#### Print Progress Bar
When enabled, the LED strip displays print progress as a visual bar:
- Lit LEDs represent completed percentage
- Configurable background color for unlit portion
- Automatically activates during print (Stage 0, RUNNING state)

#### Hardware Configuration
- Configurable data pin (GPIO 2, 4, 5, 12, 13, 16, 17, 18)
- Configurable LED count (up to 300 LEDs)
- Color order selection (GRB, RGB, BRG, etc.)
- RGBW support for compatible chipsets (SK6812-RGBW, WS2814)

#### Relay Control
Optional relay support for controlling LED strip power:
- Configurable GPIO pin
- Active LOW or HIGH (inverted) mode
- Automatically manages power based on LED state

### Architecture Changes

The codebase has been significantly refactored:
- **Header/Source Split**: All major components now have separate `.h` and `.cpp` files for better compilation and maintainability
- **Removed**: PWM channel configuration, warm/cold white support (replaced by addressable LED control)

### Configuration Persistence

LED hardware settings are stored in `/blflcconfig.json` on the device filesystem:
- Chip type, color order, LED count
- Data pin assignment
- Relay configuration
- Per-state pattern assignments

### Build Requirements

- PlatformIO with ESP32 platform
- FastLED library v3.7.7+
- ESP32 board

```bash
# Build firmware
pio run -e esp32dev

# Upload to device
pio run -e esp32dev -t upload
```

## License

Bambu Lab FastLED Controller is released under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0) license. See the [`LICENSE.md`](LICENSE.md) file for more details.

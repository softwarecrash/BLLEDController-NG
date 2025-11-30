#ifndef _LED
#define _LED

#include <Arduino.h>
#include <FastLED.h>
#include "types.h"
#include "patterns.h"

// Forward declaration - defined in mqttmanager
void controlChamberLight(bool on);

// Maximum supported LEDs
constexpr uint16_t MAX_LEDS = 300;

// LED array
extern CRGB leds[MAX_LEDS];

// Current color and pattern state
extern CRGB currentColor;
extern uint8_t currentPattern;
extern CRGB currentBgColor;

// Timing
extern unsigned long lastUpdatems;
extern unsigned long oldms;

// Timing constants
constexpr unsigned long MQTT_OFFLINE_TIMEOUT_MS = 30000;
constexpr unsigned long DOOR_DEBOUNCE_MS = 1000;
constexpr unsigned long DOOR_DOUBLE_TAP_MS = 2000;
constexpr unsigned long DOOR_INTERACTION_TIMEOUT_MS = 6000;
constexpr unsigned long LOG_DEBOUNCE_MS = 3000;

// Relay state tracking
extern bool relayCurrentState;

// Relay functions
void setupRelay();
void setRelayState(bool on);

// Color conversion
COLOR hex2rgb(String hex);

// LED setup functions
template<uint8_t DATA_PIN>
void addLedsForChipType(uint8_t chipType, uint16_t count, uint8_t colorOrder);

template<uint8_t DATA_PIN, uint8_t CLOCK_PIN>
void addLedsAPA102(uint16_t count);

void setupLeds();

// LED state functions
void setLedState(CRGB color, uint8_t pattern, CRGB bgColor = CRGB::Black);
void setLedState(const COLOR& color, uint8_t pattern, CRGB bgColor = CRGB::Black);
void setLedColor(CRGB color);
void setLedColor(const COLOR& color);
void setLedsOff();
bool areLedsOff();

// Logging functions
void printLogs(String Desc, COLOR thisColor);
void printLogs(String Desc, uint8_t r, uint8_t g, uint8_t b);

// LED State Handlers
bool handleMaintenanceMode();
bool handleWifiDebugMode();
bool handleTestColorMode();
bool handleDiscoMode();
bool handleInitialBoot();
bool handleDoorDoubleTap();
bool handleErrorStates();
bool handlePauseStates();
bool handleOffStates();
bool handleStageColors();
bool handleIdleTimeout(bool inFinishWindow);
bool handleRunningStates(bool inFinishWindow);
bool handleFinishIndication();
bool handleLedReplicationOn(bool inFinishWindow);

// Main LED update dispatcher
void updateleds();

// Main LED loop
void ledsloop();

// Template implementations must remain in header
template<uint8_t DATA_PIN>
void addLedsForChipType(uint8_t chipType, uint16_t count, uint8_t colorOrder) {
    switch (chipType) {
        case CHIP_WS2812B:
            FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, count);
            break;
        case CHIP_SK6812:
            FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, count);
            break;
        case CHIP_SK6812_RGBW:
            FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, count).setRgbw();
            break;
        case CHIP_WS2811:
            FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, count);
            break;
        case CHIP_NEOPIXEL:
            FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, count);
            break;
        case CHIP_WS2814_RGBW:
            FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, count).setRgbw(Rgbw(4000, kRGBWExactColors, W0));
            break;
        default:
            FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, count);
            break;
    }
}

template<uint8_t DATA_PIN, uint8_t CLOCK_PIN>
void addLedsAPA102(uint16_t count) {
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, count);
}

#endif

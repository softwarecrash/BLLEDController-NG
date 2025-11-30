#ifndef _PATTERNS_H
#define _PATTERNS_H

#include <FastLED.h>
#include "types.h"

// Pattern timing constants
constexpr uint16_t BREATHING_PERIOD_MS = 2000;
constexpr uint16_t BREATHING_MIN_BRIGHTNESS = 30;
constexpr uint16_t CHASE_SPEED_MS = 50;
constexpr uint8_t CHASE_TAIL_LENGTH = 5;
constexpr uint16_t RAINBOW_SPEED_MS = 20;

// Pattern state tracking
struct PatternState {
    unsigned long lastUpdate = 0;
    uint16_t position = 0;
    uint8_t brightness = 255;
    bool increasing = true;
    uint8_t hue = 0;
};

// Global pattern state
extern PatternState patternState;

// LED test sequence state
struct TestSequenceState {
    uint8_t phase = 0;
    unsigned long startTime = 0;
    bool active = false;
};

extern TestSequenceState testState;

// Convert COLOR struct to CRGB
inline CRGB colorToCRGB(const COLOR& color) {
    return CRGB(color.r, color.g, color.b);
}

// Pattern functions
void applySolidPattern(CRGB* leds, uint16_t count, CRGB color);
void applyBreathingPattern(CRGB* leds, uint16_t count, CRGB color, PatternState& state);
void applyChasePattern(CRGB* leds, uint16_t count, CRGB color, PatternState& state);
void applyRainbowPattern(CRGB* leds, uint16_t count, PatternState& state);
void applyProgressPattern(CRGB* leds, uint16_t count, CRGB color, CRGB bgColor, uint8_t progress);
void applyPattern(CRGB* leds, uint16_t count, uint8_t pattern, CRGB color,
                  PatternState& state, CRGB bgColor = CRGB::Black, uint8_t progress = 0);

// Test sequence functions
bool runTestSequence(CRGB* leds, uint16_t count, PatternState& pState);
void stopTestSequence();
bool isTestRunning();

#endif // _PATTERNS_H

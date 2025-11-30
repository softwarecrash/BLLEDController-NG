#include "patterns.h"

// Global pattern state
PatternState patternState;
TestSequenceState testState;

// Apply solid pattern - all LEDs same color
void applySolidPattern(CRGB* leds, uint16_t count, CRGB color) {
    fill_solid(leds, count, color);
}

// Apply breathing pattern - smooth brightness pulsing
void applyBreathingPattern(CRGB* leds, uint16_t count, CRGB color, PatternState& state) {
    unsigned long now = millis();
    unsigned long elapsed = now - state.lastUpdate;

    if (elapsed >= 10) {  // Update every 10ms for smooth animation
        state.lastUpdate = now;

        // Calculate position in breathing cycle (0-255)
        static uint16_t breathPhase = 0;
        breathPhase = (breathPhase + 1) % (BREATHING_PERIOD_MS / 10);

        // Sinusoidal brightness using FastLED's sin8
        // Maps 0-255 input to 0-255 output with sine curve
        uint8_t phase8 = (breathPhase * 255) / (BREATHING_PERIOD_MS / 10);
        uint8_t sinVal = sin8(phase8);

        // Scale to min-max brightness range
        state.brightness = map(sinVal, 0, 255, BREATHING_MIN_BRIGHTNESS, 255);
    }

    // Apply color with current brightness
    CRGB adjustedColor = color;
    adjustedColor.nscale8(state.brightness);
    fill_solid(leds, count, adjustedColor);
}

// Apply chase pattern - moving light along strip
void applyChasePattern(CRGB* leds, uint16_t count, CRGB color, PatternState& state) {
    unsigned long now = millis();

    if (now - state.lastUpdate >= CHASE_SPEED_MS) {
        state.lastUpdate = now;
        state.position = (state.position + 1) % count;
    }

    // Clear all LEDs first
    fill_solid(leds, count, CRGB::Black);

    // Draw chase head and tail
    for (uint8_t i = 0; i < CHASE_TAIL_LENGTH; i++) {
        int16_t pos = (state.position - i + count) % count;
        uint8_t brightness = 255 - (i * (255 / CHASE_TAIL_LENGTH));
        CRGB tailColor = color;
        tailColor.nscale8(brightness);
        leds[pos] = tailColor;
    }
}

// Apply rainbow pattern - rotating color wheel
void applyRainbowPattern(CRGB* leds, uint16_t count, PatternState& state) {
    unsigned long now = millis();

    if (now - state.lastUpdate >= RAINBOW_SPEED_MS) {
        state.lastUpdate = now;
        state.hue++;
    }

    // Fill with rainbow starting at current hue offset
    fill_rainbow(leds, count, state.hue, 255 / count);
}

// Apply progress bar pattern - LEDs light up based on print progress
void applyProgressPattern(CRGB* leds, uint16_t count, CRGB color, CRGB bgColor, uint8_t progress) {
    // Calculate how many LEDs should be lit
    uint16_t litCount = (uint16_t)((uint32_t)progress * count / 100);

    // Fill lit portion with main color
    if (litCount > 0) {
        fill_solid(leds, litCount, color);
    }

    // Fill unlit portion with background color
    if (litCount < count) {
        fill_solid(leds + litCount, count - litCount, bgColor);
    }
}

// Main pattern dispatcher
void applyPattern(CRGB* leds, uint16_t count, uint8_t pattern, CRGB color,
                  PatternState& state, CRGB bgColor, uint8_t progress) {
    switch (pattern) {
        case PATTERN_SOLID:
            applySolidPattern(leds, count, color);
            break;
        case PATTERN_BREATHING:
            applyBreathingPattern(leds, count, color, state);
            break;
        case PATTERN_CHASE:
            applyChasePattern(leds, count, color, state);
            break;
        case PATTERN_RAINBOW:
            applyRainbowPattern(leds, count, state);
            break;
        case PATTERN_PROGRESS:
            applyProgressPattern(leds, count, color, bgColor, progress);
            break;
        default:
            applySolidPattern(leds, count, color);
            break;
    }
}

// Run LED test sequence - cycles through colors and patterns
// Returns true while test is running, false when complete
bool runTestSequence(CRGB* leds, uint16_t count, PatternState& pState) {
    if (!testState.active) {
        testState.active = true;
        testState.phase = 0;
        testState.startTime = millis();
    }

    unsigned long elapsed = millis() - testState.startTime;
    const unsigned long PHASE_DURATION = 1500;  // 1.5 seconds per phase

    // Determine current phase
    uint8_t currentPhase = elapsed / PHASE_DURATION;

    if (currentPhase != testState.phase) {
        testState.phase = currentPhase;
        // Reset pattern state for new phase
        pState.position = 0;
        pState.hue = 0;
    }

    switch (testState.phase) {
        case 0:  // Red
            applySolidPattern(leds, count, CRGB::Red);
            break;
        case 1:  // Green
            applySolidPattern(leds, count, CRGB::Green);
            break;
        case 2:  // Blue
            applySolidPattern(leds, count, CRGB::Blue);
            break;
        case 3:  // White
            applySolidPattern(leds, count, CRGB::White);
            break;
        case 4:  // Chase pattern
            applyChasePattern(leds, count, CRGB::Cyan, pState);
            break;
        case 5:  // Rainbow
            applyRainbowPattern(leds, count, pState);
            break;
        case 6:  // Progress bar demo
            {
                uint8_t demoProgress = ((elapsed - (PHASE_DURATION * 6)) * 100) / PHASE_DURATION;
                applyProgressPattern(leds, count, CRGB::Green, CRGB::DarkGreen, demoProgress);
            }
            break;
        default:
            // Test complete
            testState.active = false;
            testState.phase = 0;
            return false;
    }

    return true;
}

// Stop and reset test sequence
void stopTestSequence() {
    testState.active = false;
    testState.phase = 0;
}

// Check if test is running
bool isTestRunning() {
    return testState.active;
}

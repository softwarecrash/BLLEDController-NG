#include "leds.h"
#include "logserial.h"

// LED array
CRGB leds[MAX_LEDS];

// Current color and pattern state
CRGB currentColor = CRGB::Black;
uint8_t currentPattern = PATTERN_SOLID;
CRGB currentBgColor = CRGB::Black;

// Timing
unsigned long lastUpdatems = 0;
unsigned long oldms = 0;

// Relay state tracking
bool relayCurrentState = false;

// Setup relay pin if configured
void setupRelay()
{
    if (printerConfig.relayPin >= 0)
    {
        pinMode(printerConfig.relayPin, OUTPUT);
        // Default to OFF (active low = HIGH when off, active high = LOW when off)
        digitalWrite(printerConfig.relayPin, printerConfig.relayInverted ? LOW : HIGH);
        relayCurrentState = false;
        LogSerial.printf("[Relay] Configured on GPIO %d (inverted: %s)\n",
                         printerConfig.relayPin, printerConfig.relayInverted ? "yes" : "no");
    }
}

// Set relay state (true = on, false = off)
void setRelayState(bool on)
{
    if (printerConfig.relayPin < 0)
    {
        return; // Relay not configured
    }

    if (relayCurrentState == on)
    {
        return; // No change needed
    }

    // Active LOW by default: ON = LOW, OFF = HIGH
    // If inverted: ON = HIGH, OFF = LOW
    if (printerConfig.relayInverted)
    {
        digitalWrite(printerConfig.relayPin, on ? HIGH : LOW);
    }
    else
    {
        digitalWrite(printerConfig.relayPin, on ? LOW : HIGH);
    }

    relayCurrentState = on;

    if (printerConfig.debugging || printerConfig.debugOnChange)
    {
        LogSerial.printf("[Relay] Changing state to: %s\n", on ? "ON" : "OFF");
    }
}

// Convert hex string to COLOR struct
COLOR hex2rgb(String hex)
{
    COLOR color;
    long hexcolor;
    strlcpy(color.RGBhex, hex.c_str(), sizeof(color.RGBhex));
    if (hex.charAt(0) == '#')
    {
        hex.remove(0, 1);
    }
    while (hex.length() != 6)
    {
        hex += "0";
    }
    hexcolor = strtol(hex.c_str(), NULL, 16);
    color.r = (hexcolor & 0xFF0000) >> 16;
    color.g = (hexcolor & 0x00FF00) >> 8;
    color.b = (hexcolor & 0x0000FF);

    return color;
}

void setupLeds()
{

    LogSerial.println(F("[LED] Setting up FastLED"));

    uint16_t count = min((uint16_t)printerConfig.ledConfig.ledCount, MAX_LEDS);
    uint8_t chipType = printerConfig.ledConfig.chipType;
    uint8_t dataPin = printerConfig.ledConfig.dataPin;

    // Clear any existing LED controllers
    FastLED.clear();

    // Select pin based on configuration (compile-time templates for each supported pin)
    if (chipType == CHIP_APA102) {
        // APA102 requires clock pin - use fixed pins for now
        // Data on configured pin, clock on next pin
        switch (dataPin) {
            case 2:  addLedsAPA102<2, 4>(count); break;
            case 12: addLedsAPA102<12, 14>(count); break;
            case 16: addLedsAPA102<16, 17>(count); break;
            default: addLedsAPA102<16, 17>(count); break;
        }
    } else {
        // Single-wire protocols
        uint8_t colorOrder = printerConfig.ledConfig.colorOrder;
        switch (dataPin) {
            case 2:  addLedsForChipType<2>(chipType, count, colorOrder); break;
            case 4:  addLedsForChipType<4>(chipType, count, colorOrder); break;
            case 5:  addLedsForChipType<5>(chipType, count, colorOrder); break;
            case 12: addLedsForChipType<12>(chipType, count, colorOrder); break;
            case 13: addLedsForChipType<13>(chipType, count, colorOrder); break;
            case 16: addLedsForChipType<16>(chipType, count, colorOrder); break;
            case 17: addLedsForChipType<17>(chipType, count, colorOrder); break;
            case 18: addLedsForChipType<18>(chipType, count, colorOrder); break;
            default: addLedsForChipType<16>(chipType, count, colorOrder); break;
        }
    }

    FastLED.setBrightness(printerConfig.brightness * 255 / 100);
    FastLED.clear();
    FastLED.show();

    LogSerial.printf("[LED] Configured %d LEDs on GPIO %d, chip type %d\n",
                     count, dataPin, chipType);


}

// Set current color and pattern (replaces tweenToColor)
void setLedState(CRGB color, uint8_t pattern, CRGB bgColor)
{

    currentColor = color;
    currentPattern = pattern;
    currentBgColor = bgColor;

    setRelayState(color != CRGB::Black);

}

// Overload for COLOR struct
void setLedState(const COLOR& color, uint8_t pattern, CRGB bgColor)
{
    setLedState(CRGB(color.r, color.g, color.b), pattern, bgColor);
}

// Simple color set (solid pattern, for compatibility)
void setLedColor(CRGB color)
{
    setLedState(color, PATTERN_SOLID);
}

void setLedColor(const COLOR& color)
{
    setLedState(CRGB(color.r, color.g, color.b), PATTERN_SOLID);
}

// Turn LEDs off
void setLedsOff()
{
    setLedState(CRGB::Black, PATTERN_SOLID);
    setRelayState(false);
}

// Check if LEDs are currently off
bool areLedsOff()
{
    return currentColor == CRGB::Black;
}

void printLogs(String Desc, COLOR thisColor)
{
    static COLOR lastColor = {0, 0, 0, ""};
    static String lastDesc = "";
    static unsigned long lastPrintTime = 0;

    // Skip if same state and printed less than 3 seconds ago
    if (Desc == lastDesc &&
        thisColor.r == lastColor.r &&
        thisColor.g == lastColor.g &&
        thisColor.b == lastColor.b &&
        millis() - lastPrintTime < LOG_DEBOUNCE_MS)
    {
        return;
    }

    if (printerConfig.debugging || printerConfig.debugOnChange)
    {
        LogSerial.printf("%s - Turning LEDs to:", Desc.c_str());
        if ((thisColor.r + thisColor.g + thisColor.b) == 0)
        {
            LogSerial.println(" OFF");
        }
        else
        {
            LogSerial.printf(" r: %d g: %d b: %d Brightness: %d relay: %s\n",
                             thisColor.r, thisColor.g, thisColor.b,
                             printerConfig.brightness, relayCurrentState ? "ON" : "OFF");
        }
    }

    lastColor = thisColor;
    lastDesc = Desc;
    lastPrintTime = millis();
}

void printLogs(String Desc, uint8_t r, uint8_t g, uint8_t b)
{
    COLOR tempColor;
    tempColor.r = r;
    tempColor.g = g;
    tempColor.b = b;
    printLogs(Desc, tempColor);
}

// ============================================================================
// LED State Handlers - Each returns true if state was handled
// ============================================================================

// Handle Maintenance Mode - White lights regardless of printer state
bool handleMaintenanceMode()
{
    if (!printerConfig.maintMode || !printerConfig.maintMode_update)
        return false;

    setRelayState(true);
    setLedState(CRGB::White, PATTERN_SOLID);
    printerConfig.maintMode_update = false;
    printLogs("Maintenance Mode", 255, 255, 255);
    LogSerial.printf("[%lu] ** Maintenance Mode **\n", millis());
    return true;
}

// Handle WiFi Debug Mode - Color based on signal strength
bool handleWifiDebugMode()
{
    if (!printerConfig.debugwifi)
        return false;

    if (WiFi.status() == WL_CONNECTED)
    {
        long wifiNow = WiFi.RSSI();
        if (printerConfig.debugOnChange)
        {
            LogSerial.printf("WiFi Strength Visualization: %ld dBm\n", wifiNow);
        }

        CRGB wifiColor;
        if (wifiNow >= -50)
            wifiColor = CRGB::Green;
        else if (wifiNow >= -60)
            wifiColor = CRGB(128, 255, 0);
        else if (wifiNow >= -70)
            wifiColor = CRGB::Yellow;
        else if (wifiNow >= -80)
            wifiColor = CRGB(255, 128, 0);
        else
            wifiColor = CRGB::Red;

        setRelayState(true);
        setLedState(wifiColor, printerConfig.wifiPattern);
    }
    return true;
}

// Handle Test Color Mode
bool handleTestColorMode()
{
    if (!printerConfig.testcolorEnabled || !printerConfig.testcolor_update)
        return false;

    setRelayState(true);
    setLedColor(printerConfig.testColor);
    printLogs("LED Test ON", printerConfig.testColor);
    LogSerial.printf("[%lu] ** Test Color Mode **\n", millis());
    printerConfig.testcolor_update = false;
    return true;
}

// Handle Disco Mode - Rainbow pattern
bool handleDiscoMode()
{
    if (!printerConfig.discoMode)
        return false;

    if (printerConfig.discoMode_update)
    {
        printerConfig.discoMode_update = false;
        LogSerial.printf("[%lu] ** RGB Cycle Mode **\n", millis());
    }

    if (!printerVariables.online)
    {
        setLedsOff();
        setRelayState(false);
        return true;
    }

    setRelayState(true);
    setLedState(CRGB::White, PATTERN_RAINBOW);
    return true;
}

// Handle Initial Boot state
bool handleInitialBoot()
{
    if (printerVariables.initializedLEDs)
        return false;

    printerVariables.initializedLEDs = true;
    printerConfig.inactivityStartms = millis();
    printerConfig.isIdleOFFActive = false;
    printerVariables.waitingForDoor = false;
    printerConfig.finish_check = false;
    printerVariables.lastdoorClosems = millis();
    LogSerial.println(F("Initial Boot"));
    return true;
}

// Handle Door Double-Tap toggle
bool handleDoorDoubleTap()
{
    if (!printerVariables.doorSwitchTriggered)
        return false;

    bool ledsAreOff = areLedsOff();
    bool chamberLightIsOff = !printerVariables.printerLedState;

    if (printerConfig.debugOnChange)
    {
        LogSerial.print(F("Door closed twice within 2 seconds - "));
        LogSerial.println(ledsAreOff ? F("Turning LEDs ON") : F("Turning LEDs OFF"));
    }

    if (ledsAreOff)
    {
        setRelayState(true);
        setLedState(CRGB::White, PATTERN_SOLID);
        printerConfig.isIdleOFFActive = false;
        if (printerConfig.controlChamberLight && chamberLightIsOff)
        {
            controlChamberLight(true);
        }
    }
    else
    {
        setLedsOff();
        setRelayState(false);
        printerConfig.isIdleOFFActive = true;
        printerConfig.inactivityStartms = millis() - printerConfig.inactivityTimeOut;
        if (printerConfig.controlChamberLight)
        {
            controlChamberLight(false);
        }
    }

    printerVariables.doorSwitchTriggered = false;
    return true;
}

// Handle Error States (Red indicators)
bool handleErrorStates()
{
    if (!printerConfig.errordetection)
        return false;

    // Filament runout (Stage 6)
    if (printerVariables.stage == 6 || printerVariables.overridestage == 6)
    {
        setRelayState(true);
        setLedState(printerConfig.filamentRunoutRGB, printerConfig.filamentRunoutPattern);
        printLogs("Stage 6, FILAMENT RUNOUT", printerConfig.filamentRunoutRGB);
        return true;
    }

    // Front Cover Removed (Stage 17)
    if (printerVariables.stage == 17 || printerVariables.overridestage == 17)
    {
        setRelayState(true);
        setLedState(printerConfig.frontCoverRGB, printerConfig.frontCoverPattern);
        printLogs("Stage 17, FRONT COVER REMOVED", printerConfig.frontCoverRGB);
        return true;
    }

    // Nozzle Temp Fail (Stage 20)
    if (printerVariables.stage == 20 || printerVariables.overridestage == 20)
    {
        setRelayState(true);
        setLedState(printerConfig.nozzleTempRGB, printerConfig.nozzleTempPattern);
        printLogs("Stage 20, NOZZLE TEMP FAIL", printerConfig.nozzleTempRGB);
        return true;
    }

    // Bed Temp Fail (Stage 21)
    if (printerVariables.stage == 21 || printerVariables.overridestage == 21)
    {
        setRelayState(true);
        setLedState(printerConfig.bedTempRGB, printerConfig.bedTempPattern);
        printLogs("Stage 21, BED TEMP FAIL", printerConfig.bedTempRGB);
        return true;
    }

    // HMS Serious
    if (printerVariables.parsedHMSlevel == "Serious")
    {
        setRelayState(true);
        setLedState(printerConfig.hmsSeriousRGB, printerConfig.hmsSeriousPattern);
        LogSerial.printf("HMS SERIOUS Severity - Error Code: %016llX\n", printerVariables.parsedHMScode);
        printLogs("PROBLEM", printerConfig.hmsSeriousRGB);
        return true;
    }

    // HMS Fatal
    if (printerVariables.parsedHMSlevel == "Fatal")
    {
        setRelayState(true);
        setLedState(printerConfig.hmsFatalRGB, printerConfig.hmsFatalPattern);
        LogSerial.printf("HMS FATAL Severity - Error Code: %016llX\n", printerVariables.parsedHMScode);
        printLogs("PROBLEM", printerConfig.hmsFatalRGB);
        return true;
    }

    return false;
}

// Handle Pause States (Blue indicators)
bool handlePauseStates()
{
    // Pause (by user or via Gcode)
    if ((printerVariables.stage == 16 || printerVariables.stage == 30) ||
        printerVariables.gcodeState == "PAUSE")
    {
        setRelayState(true);
        setLedState(printerConfig.pauseRGB, printerConfig.pausePattern);
        printLogs("PAUSED", printerConfig.pauseRGB);
        return true;
    }

    // First Layer Error PAUSED (Stage 34)
    if (printerVariables.stage == 34)
    {
        setRelayState(true);
        setLedState(printerConfig.firstlayerRGB, printerConfig.firstlayerPattern);
        printLogs("Stage 34, FIRST LAYER ERROR", printerConfig.firstlayerRGB);
        return true;
    }

    // Nozzle Clog PAUSED (Stage 35)
    if (printerVariables.stage == 35)
    {
        setRelayState(true);
        setLedState(printerConfig.nozzleclogRGB, printerConfig.nozzleclogPattern);
        printLogs("Stage 35, NOZZLE CLOG", printerConfig.nozzleclogRGB);
        return true;
    }

    return false;
}

// Handle Off States
bool handleOffStates()
{
    // Printer offline
    if (!printerVariables.online &&
        (millis() - printerVariables.disconnectMQTTms) >= MQTT_OFFLINE_TIMEOUT_MS)
    {
        setLedsOff();
        setRelayState(false);
        printLogs("Printer offline", 0, 0, 0);
        return true;
    }

    // Replicate printer LED OFF
    if (printerConfig.replicatestate && printerConfig.replicate_update &&
        !printerVariables.printerLedState)
    {
        setLedsOff();
        setRelayState(false);
        printLogs("LED Replication OFF", 0, 0, 0);
        printerConfig.replicate_update = false;
        return true;
    }

    return false;
}

// Handle Stage-Specific Colors
bool handleStageColors()
{
    // Cleaning nozzle (Stage 14)
    if (printerVariables.stage == 14)
    {
        setRelayState(true);
        setLedState(printerConfig.stage14Color, printerConfig.stage14Pattern);
        printLogs("Stage 14, CLEANING NOZZLE", printerConfig.stage14Color);
        return true;
    }

    // Auto Bed Leveling (Stage 1)
    if (printerVariables.stage == 1)
    {
        setRelayState(true);
        setLedState(printerConfig.stage1Color, printerConfig.stage1Pattern);
        printLogs("Stage 1, BED LEVELING", printerConfig.stage1Color);
        return true;
    }

    // Calibrating Extrusion (Stage 8)
    if (printerVariables.stage == 8)
    {
        setRelayState(true);
        setLedState(printerConfig.stage8Color, printerConfig.stage8Pattern);
        printLogs("Stage 8, CALIBRATING EXTRUSION", printerConfig.stage8Color);
        return true;
    }

    // Scanning surface (Stage 9)
    if (printerVariables.stage == 9)
    {
        setRelayState(true);
        setLedState(printerConfig.stage9Color, printerConfig.stage9Pattern);
        printLogs("Stage 9, SCANNING BED SURFACE", printerConfig.stage9Color);
        return true;
    }

    // Inspecting First Layer (Stage 10)
    if (printerVariables.stage == 10 || printerVariables.overridestage == 10)
    {
        setRelayState(true);
        setLedState(printerConfig.stage10Color, printerConfig.stage10Pattern);
        printLogs("Stage 10, FIRST LAYER INSPECTION", printerConfig.stage10Color);
        return true;
    }

    // Calibrating MicroLidar (Stage 12)
    if (printerVariables.stage == 12)
    {
        setRelayState(true);
        setLedState(printerConfig.stage10Color, printerConfig.stage10Pattern);
        printLogs("Stage 12, CALIBRATING MICRO LIDAR", printerConfig.stage10Color);
        return true;
    }

    return false;
}

// Handle Idle Timeout
bool handleIdleTimeout(bool inFinishWindow)
{
    if ((printerVariables.stage == -1 || printerVariables.stage == 255) &&
        !inFinishWindow &&
        (millis() - printerConfig.inactivityStartms) > printerConfig.inactivityTimeOut &&
        !printerConfig.isIdleOFFActive &&
        printerConfig.inactivityEnabled)
    {
        setLedsOff();
        setRelayState(false);
        controlChamberLight(false);
        printerConfig.isIdleOFFActive = true;
        if (printerConfig.debugging || printerConfig.debugOnChange)
        {
            LogSerial.printf("Idle Timeout [%d mins] - Turning LEDs OFF\n",
                             (int)(printerConfig.inactivityTimeOut / 60000));
        }
        return true;
    }
    return false;
}

// Handle Running/Active States
bool handleRunningStates(bool inFinishWindow)
{
    // Preheating Bed (Stage 2)
    if (printerVariables.stage == 2)
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("Stage 2, PREHEATING BED", printerConfig.runningColor);
        return true;
    }

    // Printing (Stage 0, RUNNING)
    if (printerVariables.stage == 0 && printerVariables.gcodeState == "RUNNING")
    {
        if (printerConfig.progressBarEnabled)
        {
            CRGB bgColor = CRGB(printerConfig.progressBarBackground.r,
                                printerConfig.progressBarBackground.g,
                                printerConfig.progressBarBackground.b);
            setRelayState(true);
            setLedState(colorToCRGB(printerConfig.runningColor), PATTERN_PROGRESS, bgColor);
        }
        else
        {
            setRelayState(true);
            setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        }
        printLogs("PRINTING", printerConfig.runningColor);
        return true;
    }

    // IDLE state
    if ((printerVariables.stage == -1 || printerVariables.stage == 255) &&
        !inFinishWindow &&
        (millis() - printerConfig.inactivityStartms < printerConfig.inactivityTimeOut))
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("IDLE", printerConfig.runningColor);
        return true;
    }

    // FAILED state
    if (printerVariables.gcodeState == "FAILED")
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("FAILED", printerConfig.runningColor);
        return true;
    }

    // PREPARE state
    if (printerVariables.gcodeState == "PREPARE")
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("PREPARE", printerConfig.runningColor);
        return true;
    }

    // Homing ToolHead (Stage 13)
    if (printerVariables.stage == 13)
    {
        LogSerial.println(F("STAGE 13, HOMING TOOL HEAD"));
        return true;
    }

    // OFFLINE state
    if (printerVariables.gcodeState == "OFFLINE" || printerVariables.stage == -2)
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("OFFLINE", printerConfig.runningColor);
        return true;
    }

    return false;
}

// Handle Finish Indication
bool handleFinishIndication()
{
    if (printerVariables.finished && printerConfig.finishIndication)
    {
        setRelayState(true);
        setLedState(printerConfig.finishColor, printerConfig.finishPattern);
        printLogs("Finished print", printerConfig.finishColor);
        printerVariables.finished = false;
        return true;
    }
    return false;
}

// Handle LED Replication ON
bool handleLedReplicationOn(bool inFinishWindow)
{
    if (printerConfig.replicatestate && printerConfig.replicate_update &&
        printerVariables.printerLedState && !inFinishWindow)
    {
        setRelayState(true);
        setLedState(printerConfig.runningColor, printerConfig.runningPattern);
        printLogs("LED Replication ON", printerConfig.runningColor);
        printerConfig.replicate_update = false;
        return true;
    }
    return false;
}

// ============================================================================
// Main LED Update Dispatcher
// ============================================================================
void updateleds()
{
    // Prevent replicate OFF immediately after door event
    if ((millis() - printerVariables.lastdoorOpenms) < DOOR_DEBOUNCE_MS ||
        (millis() - printerVariables.lastdoorClosems) < DOOR_DEBOUNCE_MS)
    {
        printerConfig.replicate_update = false;
    }

    // Priority 1: Special modes (highest priority)
    if (handleMaintenanceMode()) return;
    if (handleWifiDebugMode()) return;
    if (handleTestColorMode()) return;
    if (handleDiscoMode()) return;

    // Debug output
    if (printerConfig.debugging)
    {
        LogSerial.printf("[LED] Stage: %d | gcodeState: %s | printerLedState: %s | HMSErr: %s | ParsedHMS: %s\n",
                         printerVariables.stage,
                         printerVariables.gcodeState.c_str(),
                         printerVariables.printerLedState ? "true" : "false",
                         printerVariables.hmsstate ? "true" : "false",
                         printerVariables.parsedHMSlevel.c_str());
    }

    // Priority 2: Initial boot
    if (handleInitialBoot()) return;

    // Skip remaining handlers if in special mode
    if (printerConfig.testcolorEnabled || printerConfig.maintMode ||
        printerConfig.debugwifi || printerConfig.discoMode)
    {
        return;
    }

    // Priority 3: Door interaction
    if (handleDoorDoubleTap()) return;

    // Priority 4: Error states (red indicators)
    if (handleErrorStates()) return;

    // Priority 5: Pause states (blue indicators)
    if (handlePauseStates()) return;

    // Priority 6: Off states
    if (handleOffStates()) return;

    // Priority 7: Stage-specific colors
    if (handleStageColors()) return;

    // Calculate finish window for remaining handlers
    bool inFinishWindow = (printerConfig.finishExit && printerVariables.waitingForDoor) ||
                          (!printerConfig.finishExit && ((millis() - printerConfig.finishStartms) < printerConfig.finishTimeOut));

    // Priority 8: Idle timeout
    if (handleIdleTimeout(inFinishWindow)) return;

    // Priority 9: Running/active states
    if (handleRunningStates(inFinishWindow)) return;

    // Priority 10: Finish indication
    if (handleFinishIndication()) return;

    // Priority 11: LED replication ON
    if (handleLedReplicationOn(inFinishWindow)) return;

    // Ensure doorSwitchTriggered is processed (recursive call if needed)
    if (printerVariables.doorSwitchTriggered)
    {
        updateleds();
    }
}

void ledsloop()
{
    // Apply current pattern to LED array
    uint16_t count = min((uint16_t)printerConfig.ledConfig.ledCount, MAX_LEDS);

    // Handle test mode separately (it manages its own patterns)
    if (printerConfig.ledTestMode)
    {
        if (!runTestSequence(leds, count, patternState))
        {
            // Test complete
            printerConfig.ledTestMode = false;
            setRelayState(false);
            LogSerial.println(F("[LED] Test sequence complete"));
        }
    }
    else
    {
        setRelayState(true);
        applyPattern(leds, count, currentPattern, currentColor,
                     patternState, currentBgColor, printerVariables.printProgress);
    }

    FastLED.setBrightness(printerConfig.brightness * 255 / 100);
    FastLED.show();

    // Periodic status logging
    if ((millis() - lastUpdatems) > MQTT_OFFLINE_TIMEOUT_MS &&
        (printerConfig.maintMode || printerConfig.testcolorEnabled ||
         printerConfig.discoMode || printerConfig.debugwifi))
    {
        LogSerial.printf("[%lu] ", millis());
        if (printerConfig.maintMode)
            LogSerial.println(F("Maintenance Mode - next update in 30 seconds"));
        if (printerConfig.testcolorEnabled)
            LogSerial.println(F("Test Color - next update in 30 seconds"));
        if (printerConfig.discoMode)
            LogSerial.println(F("RGB Cycle Mode - next update in 30 seconds"));
        if (printerConfig.debugwifi)
            LogSerial.println(F("WiFi Debug Mode - next update in 30 seconds"));
        lastUpdatems = millis();
    }

    // Finish indication door interaction check
    if (printerVariables.waitingForDoor && printerConfig.finishIndication &&
        printerConfig.finishExit &&
        ((millis() - printerVariables.lastdoorClosems) < DOOR_INTERACTION_TIMEOUT_MS ||
         (millis() - printerVariables.lastdoorOpenms) < DOOR_INTERACTION_TIMEOUT_MS))
    {
        if (printerConfig.debugging || printerConfig.debugOnChange)
        {
            LogSerial.println(F("Door interaction after finish - Starting IDLE timer"));
        }
        printerVariables.waitingForDoor = false;
        printerConfig.inactivityStartms = millis();
        printerConfig.isIdleOFFActive = false;
        updateleds();
    }

    // Finish timer expiry check
    if (printerConfig.finish_check && printerConfig.finishIndication &&
        !printerConfig.finishExit &&
        ((millis() - printerConfig.finishStartms) > printerConfig.finishTimeOut))
    {
        if (printerConfig.debugging || printerConfig.debugOnChange)
        {
            LogSerial.println(F("Finish timer expired - Starting IDLE timer"));
        }
        printerConfig.finish_check = false;
        printerConfig.inactivityStartms = millis();
        printerConfig.isIdleOFFActive = false;
        updateleds();
        controlChamberLight(false);
    }

    // Inactivity timeout trigger
    if (printerConfig.inactivityEnabled &&
        (millis() - printerConfig.inactivityStartms) > printerConfig.inactivityTimeOut &&
        !printerVariables.finished &&
        !printerConfig.isIdleOFFActive)
    {
        updateleds();
    }

    // Auto turn off chamber light after timeout
    if (printerVariables.chamberLightLocked &&
        !printerVariables.doorOpen &&
        (millis() - printerConfig.inactivityStartms > printerConfig.inactivityTimeOut))
    {
        controlChamberLight(false);
        printerVariables.chamberLightLocked = false;
        if (printerConfig.debugOnChange)
            LogSerial.println(F("[LED] Timeout - Chamber light OFF and lock released"));
    }

    delay(10);
}

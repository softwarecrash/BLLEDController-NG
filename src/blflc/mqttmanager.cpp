#include "mqttmanager.h"
#include "mqttparsingutility.h"
#include "leds.h"
#include "logserial.h"

WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);

String device_topic;
String report_topic;
String clientId = "BLFLC-";

AutoGrowBufferStream stream;

unsigned long mqttattempt = 0;
unsigned long lastMQTTupdate = 0;

TaskHandle_t mqttTaskHandle = NULL;
bool mqttTaskRunning = false;
volatile bool mqttConnectInProgress = false;
bool noPrinterInfo = false;

// With a Default BLFLC
// Expected information when viewing MQTT status messages

// gcode_state	stg_cur	    BLFLC LED control	    Comments
//------------------------------------------------------------------------
// IDLE	        -1	        White	                Printer just powered on
// RUNNING	    -1	        White	                Printer sent print file
// RUNNING	     2	        White	                PREHEATING BED
// RUNNING	    14	        OFF (for Lidar)	        CLEANING NOZZLE
// RUNNING	     1	        OFF (for Lidar)	        BED LEVELING
// RUNNING	     8	        OFF (for Lidar)	        CALIBRATING EXTRUSION
// RUNNING	     0	        White	                All the printing happens here
// FINISH	    -1	        Green	                After bed is lowered and filament retracted
// FINISH	    -1	        Green	                BLFLC logic waits for a door interaction
// FINISH	    -1	        White	                After door interaction
// FINISH	    -1	        OFF                     Inactivity after 30mins

void connectMqtt()
{
    if (mqttConnectInProgress)
        return;

    mqttConnectInProgress = true;

    if (WiFi.status() != WL_CONNECTED || WiFi.getMode() != WIFI_MODE_STA)
    {
        mqttConnectInProgress = false;
        return;
    }

    if (strlen(printerConfig.printerIP) == 0 || strlen(printerConfig.accessCode) == 0)
    {
        if (noPrinterInfo == false)
        {
            Serial.println(F("[MQTT] Abort connect: printer IP or access code is wrong or empty"));
            noPrinterInfo = true;
        }
        mqttConnectInProgress = false;
        return;
    }

    if (!mqttClient.connected() && (millis() - mqttattempt) >= MQTT_RETRY_INTERVAL_MS)
    {
        // tweenToColor(10, 10, 10, 10, 10);
        Serial.println(F("Connecting to mqtt..."));

        if (mqttClient.connect(clientId.c_str(), "bblp", printerConfig.accessCode))
        {
            Serial.print(F("[MQTT] connected, subscribing to MQTT Topic:  "));
            Serial.println(report_topic);
            mqttClient.subscribe(report_topic.c_str());
            printerVariables.online = true;
            printerVariables.disconnectMQTTms = 0;
        }
        else
        {
            Serial.println(F("Failed to connect with error code: "));
            Serial.print(mqttClient.state());
            Serial.print(F("  "));
            ParseMQTTState(mqttClient.state());

            if (mqttClient.state() == 5)
            {
                setLedColor(CRGB(127, 0, 0)); // Red - MQTT connection failed
                mqttattempt = millis() - MQTT_RETRY_INTERVAL_MS;
            }
        }
    }

    mqttConnectInProgress = false;
}

void mqttTask(void *parameter)
{
    mqttTaskRunning = true;

    while (true)
    {
        if (WiFi.status() != WL_CONNECTED || WiFi.getMode() != WIFI_MODE_STA)
        {
            printerVariables.online = false;
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (!mqttClient.connected())
        {
            printerVariables.online = false;

            if (printerVariables.disconnectMQTTms == 0)
            {
                printerVariables.disconnectMQTTms = millis();
                LogSerial.println(F("[MQTT Task] Disconnected"));
                ParseMQTTState(mqttClient.state());
            }

            connectMqtt();
            vTaskDelay(pdMS_TO_TICKS(32));
        }
        else
        {
            printerVariables.disconnectMQTTms = 0;
            mqttClient.loop();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    mqttTaskRunning = false;
    vTaskDelete(NULL);
    LogSerial.printf("[MQTT Task] HighWaterMark: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
}

// ============================================================================
// MQTT Parser Functions - Each handles a specific part of the payload
// ============================================================================

// Set up JSON filter for MQTT messages
void setupMqttFilter(JsonDocument& filter)
{
    filter["print"]["command"] = true;
    filter["print"]["fail_reason"] = true;
    filter["print"]["gcode_state"] = true;
    filter["print"]["print_gcode_action"] = true;
    filter["print"]["print_real_action"] = true;
    filter["print"]["hms"] = true;
    filter["print"]["home_flag"] = true;
    filter["print"]["lights_report"] = true;
    filter["print"]["stg_cur"] = true;
    filter["print"]["print_error"] = true;
    filter["print"]["wifi_signal"] = true;
    filter["system"]["command"] = true;
    filter["system"]["led_mode"] = true;
}

// Check if command should be skipped (noise filtering)
bool shouldSkipCommand(JsonDocument& msg)
{
    if (msg["print"]["command"].isNull())
        return false;

    const char* cmd = msg["print"]["command"];
    return (strcmp(cmd, "gcode_line") == 0 ||
            strcmp(cmd, "project_prepare") == 0 ||
            strcmp(cmd, "project_file") == 0 ||
            strcmp(cmd, "clean_print_error") == 0 ||
            strcmp(cmd, "resume") == 0 ||
            strcmp(cmd, "get_accessories") == 0 ||
            strcmp(cmd, "prepare") == 0 ||
            strcmp(cmd, "extrusion_cali_get") == 0);
}

// Check if in special mode (maintenance, test, disco, wifi debug)
bool isInSpecialMode()
{
    return printerConfig.maintMode || printerConfig.testcolorEnabled ||
           printerConfig.discoMode || printerConfig.debugwifi;
}

// Handle door opened event
void handleDoorOpened()
{
    printerVariables.lastdoorOpenms = millis();

    // If light is off, turn it on and lock it
    if (printerConfig.controlChamberLight && !printerVariables.printerLedState)
    {
        printerVariables.chamberLightLocked = true;
        printerVariables.printerLedState = true;
        printerConfig.replicate_update = false;
        controlChamberLight(true);
        printerVariables.stage = 255;
        LogSerial.println(F("[MQTT] Door opened – Light forced ON"));
    }

    // Restart inactivity timer
    printerConfig.inactivityStartms = millis();
    printerConfig.isIdleOFFActive = false;
}

// Handle door closed event
void handleDoorClosed()
{
    printerVariables.lastdoorClosems = millis();

    // Release chamber light lock
    if (printerConfig.controlChamberLight)
    {
        printerVariables.chamberLightLocked = false;
    }

    // Turn off LED bar immediately if inactivity disabled
    if (!printerConfig.inactivityEnabled)
    {
        printerVariables.printerLedState = false;
        printerConfig.replicate_update = false;
        printerVariables.stage = 999;
        setLedsOff();
        controlChamberLight(false);
        LogSerial.println(F("[MQTT] Door closed – LED bar OFF (inactivity disabled)"));
    }

    // Reset inactivity timer
    printerConfig.inactivityStartms = millis();
    printerConfig.isIdleOFFActive = false;

    // Double-close detection for toggle
    if ((millis() - printerVariables.lastdoorOpenms) < DOOR_DOUBLE_TAP_MS)
    {
        printerVariables.doorSwitchTriggered = true;
    }
}

// Parse door status from home_flag
bool parseDoorStatus(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["home_flag"].isNull())
        return false;

    long homeFlag = msg["print"]["home_flag"];
    bool doorState = bitRead(homeFlag, 23);  // Bit 23 = door open

    if (printerVariables.doorOpen == doorState)
        return false;

    printerVariables.doorOpen = doorState;

    if (printerConfig.debugOnChange)
    {
        LogSerial.print(F("[MQTT] Door "));
        LogSerial.println(doorState ? F("Opened") : F("Closed"));
    }

    if (doorState)
    {
        handleDoorOpened();
    }
    else
    {
        handleDoorClosed();
    }

    changed = true;
    updateleds();
    return true;
}

// Parse printer stage (stg_cur)
bool parseStage(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["stg_cur"].isNull())
        return false;

    int newStage = msg["print"]["stg_cur"].as<int>();
    if (printerVariables.stage == newStage)
        return false;

    printerVariables.stage = newStage;

    if (printerConfig.debugOnChange || printerConfig.debugging)
    {
        LogSerial.print(F("[MQTT] update - stg_cur now: "));
        LogSerial.println(printerVariables.stage);
    }

    changed = true;
    return true;
}

// Parse gcode state
bool parseGcodeState(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["gcode_state"].isNull())
        return false;

    if ((millis() - lastMQTTupdate) <= MQTT_STATUS_DEBOUNCE_MS)
        return false;

    String mqttgcodeState = msg["print"]["gcode_state"].as<String>();

    // Keep inactivity timer running during active states
    if (mqttgcodeState == "RUNNING" || mqttgcodeState == "PAUSE")
    {
        printerConfig.inactivityStartms = millis();
    }

    // Turn on chamber light at print start
    if (mqttgcodeState == "RUNNING" && printerConfig.controlChamberLight &&
        !printerVariables.printerLedState)
    {
        controlChamberLight(true);
        LogSerial.println(F("[MQTT] Print started – Chamber Light ON requested"));
    }

    // Handle state change
    if (printerVariables.gcodeState == mqttgcodeState)
        return false;

    if (mqttgcodeState == "RUNNING")
    {
        printerVariables.overridestage = 999;  // Reset HMS override
    }

    if (mqttgcodeState == "FINISH")
    {
        printerVariables.finished = true;
        printerVariables.waitingForDoor = true;
        printerConfig.finishStartms = millis();
        printerConfig.finish_check = true;
    }

    printerVariables.gcodeState = mqttgcodeState;

    if (printerConfig.debugOnChange || printerConfig.debugging)
    {
        LogSerial.print(F("[MQTT] update - gcode_state now: "));
        LogSerial.println(printerVariables.gcodeState);
    }

    changed = true;
    return true;
}

// Parse manual pause command
bool parsePauseCommand(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["command"].isNull())
        return false;

    if (msg["print"]["command"] != "pause")
        return false;

    lastMQTTupdate = millis();
    LogSerial.println(F("[MQTT] update - manual PAUSE"));
    printerVariables.gcodeState = "PAUSE";
    changed = true;
    return true;
}

// Parse lights report (chamber light status)
bool parseLightsReport(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["lights_report"].isNull())
        return false;

    if ((millis() - lastMQTTupdate) <= MQTT_STATUS_DEBOUNCE_MS)
        return false;

    JsonArray lightsReport = msg["print"]["lights_report"];
    for (JsonObject light : lightsReport)
    {
        if (light["node"] != "chamber_light")
            continue;

        bool newState = (light["mode"] == "on");
        if (printerVariables.printerLedState == newState)
            continue;

        printerVariables.printerLedState = newState;
        printerConfig.replicate_update = true;

        if (printerConfig.debugOnChange || printerConfig.debugging)
        {
            LogSerial.print(F("[MQTT] chamber_light now: "));
            LogSerial.println(printerVariables.printerLedState);
        }

        if (printerVariables.waitingForDoor && printerConfig.finish_check)
        {
            printerVariables.finished = true;
        }

        changed = true;
    }
    return changed;
}

// Parse system LED control commands
bool parseSystemCommand(JsonDocument& msg, bool& changed)
{
    if (msg["system"]["command"].isNull())
        return false;

    if (msg["system"]["command"] != "ledctrl")
        return false;

    bool newState = (msg["system"]["led_mode"] == "on");
    if (printerVariables.printerLedState == newState)
        return false;

    printerVariables.printerLedState = newState;
    printerConfig.replicate_update = true;
    lastMQTTupdate = millis();

    if (printerConfig.debugOnChange || printerConfig.debugging)
    {
        LogSerial.print(F("[MQTT] led_mode now: "));
        LogSerial.println(printerVariables.printerLedState);
    }

    if (printerVariables.waitingForDoor && printerConfig.finish_check)
    {
        printerVariables.finished = true;
    }

    changed = true;
    return true;
}

// Map HMS codes to stage overrides
void applyHMSOverride(uint64_t code)
{
    // HMS code to stage mappings
    struct HMSMapping {
        uint64_t code;
        int stage;
    };

    static const HMSMapping mappings[] = {
        {0x0C0003000003000B, 10},  // First layer inspection
        {0x0300120000020001, 17},  // Front cover removed
        {0x0700200000030001, 6},   // Filament runout
        {0x0300020000010001, 20},  // Nozzle temp fail
        {0x0300010000010007, 21},  // Bed temp fail
    };

    for (const auto& mapping : mappings)
    {
        if (code == mapping.code)
        {
            printerVariables.overridestage = mapping.stage;
            return;
        }
    }
}

// Parse HMS (Health Management System) errors
bool parseHMS(JsonDocument& msg, bool& changed)
{
    if (msg["print"]["hms"].isNull())
        return false;

    String oldHMSlevel = printerVariables.parsedHMSlevel;

    // Normalize ignore list once before the loop
    String normalizedIgnoreList = printerConfig.hmsIgnoreList;
    normalizedIgnoreList.replace("-", "_");
    normalizedIgnoreList.replace(" ", "");
    normalizedIgnoreList.replace("\r", "");
    normalizedIgnoreList.replace("\n", ",");
    String wrappedIgnoreList = "," + normalizedIgnoreList + ",";

    printerVariables.hmsstate = false;
    printerVariables.parsedHMSlevel = "";

    for (const auto& hms : msg["print"]["hms"].as<JsonArray>())
    {
        uint64_t code = ((uint64_t)hms["attr"] << 32) + (uint64_t)hms["code"];

        char strHMScode[32];
        formatHMSCode(code, strHMScode, sizeof(strHMScode));

        // Check ignore list
        if (wrappedIgnoreList.indexOf("," + String(strHMScode) + ",") >= 0)
        {
            LogSerial.print(F("[MQTT] Ignored HMS Code: "));
            LogSerial.println(strHMScode);
            continue;
        }

        String severity = ParseHMSSeverity(hms["code"]);
        if (severity != "")
        {
            printerVariables.hmsstate = true;
            printerVariables.parsedHMSlevel = severity;
            printerVariables.parsedHMScode = code;
        }
    }

    if (oldHMSlevel == printerVariables.parsedHMSlevel)
        return false;

    // Apply stage override based on HMS code
    applyHMSOverride(printerVariables.parsedHMScode);

    // Debug logging
    if (printerConfig.debugging || printerConfig.debugOnChange)
    {
        LogSerial.print(F("[MQTT] update - parsedHMSlevel now: "));
        if (printerVariables.parsedHMSlevel.length() > 0)
        {
            LogSerial.print(printerVariables.parsedHMSlevel);
            LogSerial.print(F("      Error Code: HMS_"));
            char strHMScode[24];
            formatHMSCodeShort(printerVariables.parsedHMScode, strHMScode, sizeof(strHMScode));
            LogSerial.print(strHMScode);
            if (printerVariables.overridestage != printerVariables.stage)
            {
                LogSerial.println(F(" **"));
            }
            else
            {
                LogSerial.println();
            }
        }
        else
        {
            LogSerial.println(F("NULL"));
            printerVariables.overridestage = 999;
        }
    }

    changed = true;
    return true;
}

// Apply changes after parsing (reset timers, update LEDs)
void applyMqttChanges()
{
    printerConfig.inactivityStartms = millis();
    printerConfig.isIdleOFFActive = false;

    if (printerConfig.debugging)
    {
        LogSerial.println(F("Change from mqtt"));
    }

    printerConfig.maintMode_update = true;
    printerConfig.discoMode_update = true;
    printerConfig.replicate_update = true;
    printerConfig.testcolor_update = true;

    updateleds();
}

// ============================================================================
// Main MQTT Parse Callback - Dispatcher
// ============================================================================
void ParseCallback(char *topic, byte *payload, unsigned int length)
{
    JsonDocument messageobject;
    JsonDocument filter;

    // Set up filter for relevant fields
    setupMqttFilter(filter);

    auto deserializeError = deserializeJson(messageobject, payload, length,
                                            DeserializationOption::Filter(filter));
    if (deserializeError)
    {
        LogSerial.println(F("Deserialize error while parsing mqtt"));
        return;
    }

    // Debug logging
    if (printerConfig.debugging)
    {
        LogSerial.print(F("MQTT message received. "));
        LogSerial.print(F("Free Heap: "));
        LogSerial.println(ESP.getFreeHeap());
    }

    // Early exit for noise commands
    if (shouldSkipCommand(messageobject))
        return;

    // Early exit for empty messages
    if (messageobject.size() == 0)
        return;

    // Debug: show filtered message
    if (printerConfig.mqttdebug)
    {
        LogSerial.print(F("(Filtered) MQTT payload, ["));
        LogSerial.print(stream.current_length());
        LogSerial.print(F("], "));
        serializeJson(messageobject, LogSerial);
        LogSerial.println();
    }

    // Skip processing in special modes (but allow debug output above)
    if (printerConfig.mqttdebug && isInSpecialMode())
    {
        LogSerial.print(F("[MQTT] Message Ignored while in "));
        if (printerConfig.maintMode) LogSerial.print(F("Maintenance"));
        if (printerConfig.testcolorEnabled) LogSerial.print(F("Test Color"));
        if (printerConfig.discoMode) LogSerial.print(F("RGB Cycle"));
        if (printerConfig.debugwifi) LogSerial.print(F("Wifi Debug"));
        LogSerial.println(F(" mode"));
        return;
    }

    bool changed = false;

    // Parse each section of the message
    parseDoorStatus(messageobject, changed);
    parseStage(messageobject, changed);
    parseGcodeState(messageobject, changed);
    parsePauseCommand(messageobject, changed);
    parseLightsReport(messageobject, changed);
    parseSystemCommand(messageobject, changed);
    parseHMS(messageobject, changed);

    // Apply changes if any parser detected a change
    if (changed)
    {
        applyMqttChanges();
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    ParseCallback(topic, (byte *)stream.get_buffer(), stream.current_length());
    stream.flush();
}

void controlChamberLight(bool on)
{
    LogSerial.printf("[DEBUG] controlChamberLight called with: %s\n", on ? "true" : "false");
    if (!printerConfig.controlChamberLight)
        return;
    printerVariables.printerLedState = on; // <-- Set state flag to avoid replicate overwrite
    if (!mqttClient.connected())
    {
        LogSerial.println(F("[MQTT] Skipped chamber_light control – MQTT not connected"));
        return;
    }

    JsonDocument doc;
    JsonObject system = doc["system"].to<JsonObject>();
    system["command"] = "ledctrl";
    system["sequence_id"] = "blflc_auto";
    system["led_node"] = "chamber_light";
    system["led_mode"] = on ? "on" : "off";
    system["led_on_time"] = 500;
    system["led_off_time"] = 500;
    system["loop_times"] = 0;
    system["interval_time"] = 1000;

    String payload;
    serializeJson(doc, payload);

    String topic = String("device/") + printerConfig.serialNumber + "/request";
    mqttClient.publish(topic.c_str(), payload.c_str());

    LogSerial.print(F("[MQTT] Chamber Light "));
    LogSerial.print(on ? F("ON") : F("OFF"));
    LogSerial.print(F(" sent to topic: "));
    LogSerial.println(topic);
}

void setupMqtt()
{
    clientId += String(random(0xffff), HEX);
    LogSerial.print(F("Setting up MQTT with Bambu Lab Printer IP address: "));
    LogSerial.println(printerConfig.printerIP);

    device_topic = String("device/") + printerConfig.serialNumber;
    report_topic = device_topic + String("/report");

    wifiSecureClient.setInsecure();
    wifiSecureClient.setTimeout(15);
    mqttClient.setSocketTimeout(17);
    mqttClient.setBufferSize(1024);
    mqttClient.setServer(printerConfig.printerIP, 8883);
    mqttClient.setStream(stream);
    mqttClient.setCallback(mqttCallback);

    LogSerial.println(F("Finished setting up MQTT"));

    if (mqttTaskHandle == NULL)
    {
        BaseType_t result;

#if CONFIG_FREERTOS_UNICORE
        result = xTaskCreate(
            mqttTask,
            "mqttTask",
            TASK_RAM,
            NULL,
            1,
            &mqttTaskHandle);
#else
        result = xTaskCreatePinnedToCore(
            mqttTask,
            "mqttTask",
            TASK_RAM,
            NULL,
            1,
            &mqttTaskHandle,
            1 // Core 1 (App Core)
        );
#endif

        if (result == pdPASS)
        {
            LogSerial.println(F("[MQTT] task successfully started"));
        }
        else
        {
            LogSerial.println(F("Failed to create MQTT task!"));
        }
    }
}

void mqttloop()
{
    if (WiFi.status() != WL_CONNECTED || WiFi.getMode() != WIFI_MODE_STA)
    {
        // Abort MQTT connection attempt when no Wifi
        return;
    }
    if (!mqttClient.connected())
    {
        printerVariables.online = false;
        // Only sent the timer from the first instance of a MQTT disconnect
        if (printerVariables.disconnectMQTTms == 0)
        {
            printerVariables.disconnectMQTTms = millis();
            // Record last time MQTT dropped connection
            LogSerial.println(F("[MQTT] dropped during mqttloop"));
            ParseMQTTState(mqttClient.state());
        }
        // delay(500);
        connectMqtt();
        delay(32);
        return;
    }
    else
    {
        printerVariables.disconnectMQTTms = 0;
    }
    mqttClient.loop();
    delay(10);
}

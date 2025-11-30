#ifndef _MQTTMANAGER
#define _MQTTMANAGER

#define TASK_RAM 20480 // 20kB for MQTT Task Stack Size

// Timing constants
constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 3000;
constexpr unsigned long MQTT_STATUS_DEBOUNCE_MS = 3000;

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "autogrowbufferstream.h"
#include "types.h"

// MQTT client instances
extern WiFiClientSecure wifiSecureClient;
extern PubSubClient mqttClient;

// MQTT topics
extern String device_topic;
extern String report_topic;
extern String clientId;

// Stream buffer
extern AutoGrowBufferStream stream;

// MQTT timing
extern unsigned long mqttattempt;
extern unsigned long lastMQTTupdate;

// Task management
extern TaskHandle_t mqttTaskHandle;
extern bool mqttTaskRunning;
extern volatile bool mqttConnectInProgress;

// MQTT connection functions
void connectMqtt();
void mqttTask(void *parameter);

// MQTT filter setup
void setupMqttFilter(JsonDocument& filter);

// Command filtering
bool shouldSkipCommand(JsonDocument& msg);
bool isInSpecialMode();

// Door event handlers
void handleDoorOpened();
void handleDoorClosed();

// Parser functions
bool parseDoorStatus(JsonDocument& msg, bool& changed);
bool parseStage(JsonDocument& msg, bool& changed);
bool parseGcodeState(JsonDocument& msg, bool& changed);
bool parsePauseCommand(JsonDocument& msg, bool& changed);
bool parseLightsReport(JsonDocument& msg, bool& changed);
bool parseSystemCommand(JsonDocument& msg, bool& changed);
void applyHMSOverride(uint64_t code);
bool parseHMS(JsonDocument& msg, bool& changed);
void applyMqttChanges();

// Main callback functions
void ParseCallback(char *topic, byte *payload, unsigned int length);
void mqttCallback(char *topic, byte *payload, unsigned int length);

// Chamber light control
void controlChamberLight(bool on);

// Setup and loop functions
void setupMqtt();
void mqttloop();

#endif

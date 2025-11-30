#ifndef _BLFLC_IMPROV_SERIAL_H
#define _BLFLC_IMPROV_SERIAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <improv.h>
#include "types.h"
#include "filesystem.h"

// Improv Serial state
static improv::State improvState = improv::STATE_STOPPED;
static improv::Error improvError = improv::ERROR_NONE;
static bool improvActive = false;
static unsigned long improvLastActivity = 0;
static const unsigned long IMPROV_TIMEOUT_MS = 60000; // 60 second timeout for provisioning

// Buffer for Improv serial parsing
static uint8_t improvBuffer[128];
static size_t improvBufferPos = 0;

// Forward declarations
void sendImprovStateResponse(uint8_t state, bool sendError = true);
void sendImprovErrorResponse(improv::Error error);
void sendImprovRPCResponse(improv::Command cmd, const std::vector<String> &data);
std::vector<uint8_t> buildImprovSerialPacket(improv::ImprovSerialType type, const std::vector<uint8_t> &data);

// Build and send an Improv serial packet
void sendImprovPacket(const std::vector<uint8_t> &packet) {
    for (auto byte : packet) {
        Serial.write(byte);
    }
}

// Build Improv serial packet with header
std::vector<uint8_t> buildImprovSerialPacket(improv::ImprovSerialType type, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> packet;
    packet.reserve(9 + data.size());

    // Header: "IMPROV"
    packet.push_back('I');
    packet.push_back('M');
    packet.push_back('P');
    packet.push_back('R');
    packet.push_back('O');
    packet.push_back('V');

    // Version
    packet.push_back(improv::IMPROV_SERIAL_VERSION);

    // Type
    packet.push_back(static_cast<uint8_t>(type));

    // Length
    packet.push_back(static_cast<uint8_t>(data.size()));

    // Data
    for (auto byte : data) {
        packet.push_back(byte);
    }

    // Checksum (sum of all bytes)
    uint8_t checksum = 0;
    for (auto byte : packet) {
        checksum += byte;
    }
    packet.push_back(checksum);

    return packet;
}

// Send current state response
void sendImprovStateResponse(uint8_t state, bool sendError) {
    std::vector<uint8_t> data = {state};
    auto packet = buildImprovSerialPacket(improv::TYPE_CURRENT_STATE, data);
    sendImprovPacket(packet);

    if (sendError) {
        sendImprovErrorResponse(improvError);
    }
}

// Send error state response
void sendImprovErrorResponse(improv::Error error) {
    std::vector<uint8_t> data = {static_cast<uint8_t>(error)};
    auto packet = buildImprovSerialPacket(improv::TYPE_ERROR_STATE, data);
    sendImprovPacket(packet);
}

// Build RPC response data
std::vector<uint8_t> buildRPCResponseData(improv::Command cmd, const std::vector<String> &strings) {
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(cmd));

    // Calculate total length
    size_t totalLen = 0;
    for (const auto &str : strings) {
        totalLen += 1 + str.length(); // length byte + string bytes
    }
    data.push_back(static_cast<uint8_t>(totalLen));

    // Add strings with length prefix
    for (const auto &str : strings) {
        data.push_back(static_cast<uint8_t>(str.length()));
        for (size_t i = 0; i < str.length(); i++) {
            data.push_back(str[i]);
        }
    }

    return data;
}

// Send RPC response
void sendImprovRPCResponse(improv::Command cmd, const std::vector<String> &strings) {
    auto data = buildRPCResponseData(cmd, strings);
    auto packet = buildImprovSerialPacket(improv::TYPE_RPC_RESPONSE, data);
    sendImprovPacket(packet);
}

// Get device info for Improv
std::vector<String> getImprovDeviceInfo() {
    std::vector<String> info;
    info.push_back("BLFLC Controller");  // Firmware name
    info.push_back(globalVariables.FWVersion);  // Firmware version
    info.push_back("ESP32");  // Hardware chip/variant
    info.push_back(globalVariables.Host);  // Device name
    return info;
}

// Handle WiFi connection request from Improv
bool handleImprovWifiConnect(const char* ssid, const char* password) {
    Serial.printf("[Improv] Connecting to WiFi: %s\n", ssid);

    // Update state to provisioning
    improvState = improv::STATE_PROVISIONING;
    improvError = improv::ERROR_NONE;
    sendImprovStateResponse(improvState);

    // Disconnect if already connected
    WiFi.disconnect();
    delay(100);

    // Attempt connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait for connection with timeout
    unsigned long startTime = millis();
    const unsigned long timeout = 15000; // 15 second timeout

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Improv] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

        // Save credentials to filesystem
        strncpy(globalVariables.SSID, ssid, sizeof(globalVariables.SSID) - 1);
        globalVariables.SSID[sizeof(globalVariables.SSID) - 1] = '\0';
        strncpy(globalVariables.APPW, password, sizeof(globalVariables.APPW) - 1);
        globalVariables.APPW[sizeof(globalVariables.APPW) - 1] = '\0';
        saveFileSystem();

        // Update state to provisioned
        improvState = improv::STATE_PROVISIONED;
        improvError = improv::ERROR_NONE;
        sendImprovStateResponse(improvState);

        // Send RPC response with redirect URL
        std::vector<String> urls;
        urls.push_back("http://" + WiFi.localIP().toString());
        sendImprovRPCResponse(improv::WIFI_SETTINGS, urls);

        return true;
    } else {
        Serial.println("[Improv] Connection failed!");

        // Update state back to authorized with error
        improvState = improv::STATE_AUTHORIZED;
        improvError = improv::ERROR_UNABLE_TO_CONNECT;
        sendImprovStateResponse(improvState);

        return false;
    }
}

// Handle incoming Improv command
void handleImprovCommand(const improv::ImprovCommand &cmd) {
    improvLastActivity = millis();

    switch (cmd.command) {
        case improv::WIFI_SETTINGS: {
            handleImprovWifiConnect(cmd.ssid.c_str(), cmd.password.c_str());
            break;
        }

        case improv::GET_CURRENT_STATE: {
            sendImprovStateResponse(improvState);
            break;
        }

        case improv::GET_DEVICE_INFO: {
            auto info = getImprovDeviceInfo();
            sendImprovRPCResponse(improv::GET_DEVICE_INFO, info);
            break;
        }

        case improv::GET_WIFI_NETWORKS: {
            // Scan for networks
            Serial.println("[Improv] Scanning WiFi networks...");
            int numNetworks = WiFi.scanNetworks();

            for (int i = 0; i < numNetworks && i < 10; i++) {
                std::vector<String> network;
                network.push_back(WiFi.SSID(i));
                network.push_back(String(WiFi.RSSI(i)));
                network.push_back(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "NO" : "YES");
                sendImprovRPCResponse(improv::GET_WIFI_NETWORKS, network);
            }

            // Send empty response to indicate end of list
            sendImprovRPCResponse(improv::GET_WIFI_NETWORKS, std::vector<String>());
            break;
        }

        default: {
            improvError = improv::ERROR_UNKNOWN_RPC;
            sendImprovErrorResponse(improvError);
            break;
        }
    }
}

// Improv serial byte parser callback - returns true if command was handled
bool onImprovCommandCallback(improv::ImprovCommand cmd) {
    handleImprovCommand(cmd);
    return true;
}

// Improv error callback
void onImprovErrorCallback(improv::Error error) {
    improvError = error;
    sendImprovErrorResponse(error);
}

// Setup Improv Serial
void setupImprovSerial() {
    improvActive = true;
    improvLastActivity = millis();

    // Set initial state based on whether we already have credentials
    if (WiFi.status() == WL_CONNECTED) {
        improvState = improv::STATE_PROVISIONED;
    } else {
        // No authorization needed for serial Improv
        improvState = improv::STATE_AUTHORIZED;
    }

    Serial.println("[Improv] Serial provisioning ready");
    sendImprovStateResponse(improvState);
}

// Process Improv Serial in loop - must be called frequently
void loopImprovSerial() {
    if (!improvActive) {
        return;
    }

    // Process any available serial bytes
    while (Serial.available()) {
        uint8_t byte = Serial.read();

        // Store byte in buffer
        if (improvBufferPos < sizeof(improvBuffer)) {
            improvBuffer[improvBufferPos] = byte;
        }

        if (improv::parse_improv_serial_byte(
                improvBufferPos,
                byte,
                improvBuffer,
                [](improv::ImprovCommand cmd) -> bool {
                    handleImprovCommand(cmd);
                    return true;
                },
                [](improv::Error error) {
                    improvError = error;
                    sendImprovErrorResponse(error);
                })) {
            // Command was parsed successfully, reset buffer
            improvBufferPos = 0;
            improvLastActivity = millis();
        } else {
            // Increment position for next byte
            improvBufferPos++;
            // Reset if buffer overflows
            if (improvBufferPos >= sizeof(improvBuffer)) {
                improvBufferPos = 0;
            }
        }
    }
}

// Check if Improv is currently active
bool isImprovActive() {
    return improvActive;
}

// Stop Improv Serial (e.g., after successful provisioning)
void stopImprovSerial() {
    improvActive = false;
    improvState = improv::STATE_STOPPED;
    Serial.println("[Improv] Serial provisioning stopped");
}

#endif // _BLFLC_IMPROV_SERIAL_H

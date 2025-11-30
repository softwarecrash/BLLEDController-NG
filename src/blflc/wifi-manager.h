#ifndef _BLFLCWIFI_MANAGER
#define _BLFLCWIFI_MANAGER

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

// DNS server for captive portal
extern DNSServer dnsServer;
extern IPAddress apIP;

// WiFi state variables
extern bool shouldSaveConfig;
extern int connectionAttempts;
extern int wifimode;
extern uint8_t bssid[6];

// Callback for config mode
void configModeCallback();

// Convert WiFi status to string
const char* wl_status_to_string(wl_status_t status);

// Convert MAC address string to bytes
int str2mac(const char* mac, uint8_t* values);

// Connect to WiFi network
bool connectToWifi();

// Start AP mode for configuration
void startAPMode();

// Scan for available networks
void scanNetwork();

#endif

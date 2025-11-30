#ifndef _MQTTPARSERUTILITY
#define _MQTTPARSERUTILITY

#include <Arduino.h>

// Format a 64-bit HMS code as "HMS_XXXX_XXXX_XXXX_XXXX" string
// Buffer must be at least 24 bytes
void formatHMSCode(uint64_t code, char* buffer, size_t bufferSize);

// Format without "HMS_" prefix (for logging)
void formatHMSCodeShort(uint64_t code, char* buffer, size_t bufferSize);

// Parse HMS severity level from code
String ParseHMSSeverity(int code);

// Parse and log MQTT connection state
void ParseMQTTState(int code);

#endif

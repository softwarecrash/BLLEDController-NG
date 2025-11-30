#include "mqttparsingutility.h"
#include "logserial.h"

// Format a 64-bit HMS code as "HMS_XXXX_XXXX_XXXX_XXXX" string
// Buffer must be at least 24 bytes
void formatHMSCode(uint64_t code, char* buffer, size_t bufferSize) {
    int chunk1 = (code >> 48);
    int chunk2 = (code >> 32) & 0xFFFF;
    int chunk3 = (code >> 16) & 0xFFFF;
    int chunk4 = code & 0xFFFF;
    snprintf(buffer, bufferSize, "HMS_%04X_%04X_%04X_%04X", chunk1, chunk2, chunk3, chunk4);
}

// Format without "HMS_" prefix (for logging)
void formatHMSCodeShort(uint64_t code, char* buffer, size_t bufferSize) {
    int chunk1 = (code >> 48);
    int chunk2 = (code >> 32) & 0xFFFF;
    int chunk3 = (code >> 16) & 0xFFFF;
    int chunk4 = code & 0xFFFF;
    snprintf(buffer, bufferSize, "%04X_%04X_%04X_%04X", chunk1, chunk2, chunk3, chunk4);
}

String ParseHMSSeverity(int code) { // Provided by WolfWithSword
    int parsedcode (code>>16);
    switch (parsedcode){
        case 1:
            return F("Fatal");
        case 2:
            return F("Serious");
        case 3:
            return F("Common");
        case 4:
            return F("Info");
        default:;
    }
    return "";
}

void ParseMQTTState(int code) {
    switch (code)
    {
    case -4: // MQTT_CONNECTION_TIMEOUT
        LogSerial.println(F("[MQTT] Timeout (-4)"));
        break;
    case -3: // MQTT_CONNECTION_LOST
        LogSerial.println(F("[MQTT] Connection Lost (-3)"));
        break;
    case -2: // MQTT_CONNECT_FAILED
        LogSerial.println(F("[MQTT] Connection Failed (-2)"));
        break;
    case -1: // MQTT_DISCONNECTED
        LogSerial.println(F("[MQTT] Disconnected (-1)"));
        break;
    case 0:  // MQTT_CONNECTED
        LogSerial.println(F("[MQTT] Connected (0)"));
        break;
    case 1:  // MQTT_CONNECT_BAD_PROTOCOL
        LogSerial.println(F("[MQTT] Bad protocol (1)"));
        break;
    case 2:  // MQTT_CONNECT_BAD_CLIENT_ID
        LogSerial.println(F("[MQTT] Bad Client ID (2)"));
        break;
    case 3:  // MQTT_CONNECT_UNAVAILABLE
        LogSerial.println(F("[MQTT] Unavailable (3)"));
        break;
    case 4:  // MQTT_CONNECT_BAD_CREDENTIALS
        LogSerial.println(F("[MQTT] Bad Credentials (4)"));
        break;
    case 5: // MQTT UNAUTHORIZED
        LogSerial.println(F("[MQTT] Unauthorized (5)"));
        break;
    }
}

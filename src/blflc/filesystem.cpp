#include "filesystem.h"
#include <WiFi.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "types.h"
#include "logserial.h"
#include "leds.h"

const char *configPath = "/blflcconfig.json";

char *generateRandomString(int length)
{
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int charsetLength = strlen(charset);

    char *randomString = new char[length + 1];
    for (int i = 0; i < length; i++)
    {
        int randomIndex = random(0, charsetLength);
        randomString[i] = charset[randomIndex];
    }
    randomString[length] = '\0';

    return randomString;
}

void saveFileSystem()
{
    LogSerial.println(F("[Filesystem] Saving config"));

    JsonDocument json;
    json["ssid"] = globalVariables.SSID;
    json["appw"] = globalVariables.APPW;
    json["HTTPUser"] = securityVariables.HTTPUser;
    json["HTTPPass"] = securityVariables.HTTPPass;
    json["printerIp"] = printerConfig.printerIP;
    json["accessCode"] = printerConfig.accessCode;
    json["serialNumber"] = printerConfig.serialNumber;
    // json["webpagePassword"] = printerConfig.webpagePassword;
    json["bssi"] = printerConfig.BSSID;
    json["brightness"] = printerConfig.brightness;
    // LED Behaviour (Choose One)
    json["maintMode"] = printerConfig.maintMode;
    json["discoMode"] = printerConfig.discoMode;
    json["replicatestate"] = printerConfig.replicatestate;
    // Running Color
    json["runningRGB"] = printerConfig.runningColor.RGBhex;
    // Test LED Colors
    json["showtestcolor"] = printerConfig.testcolorEnabled;
    json["testRGB"] = printerConfig.testColor.RGBhex;
    json["debugwifi"] = printerConfig.debugwifi;
    // Options
    json["finishIndication"] = printerConfig.finishIndication;
    json["finishColor"] = printerConfig.finishColor.RGBhex;
    json["finishExit"] = printerConfig.finishExit;
    json["finish_check"] = printerConfig.finish_check;
    json["finishTimerMins"] = printerConfig.finishTimeOut;
    json["inactivityEnabled"] = printerConfig.inactivityEnabled;
    json["inactivityTimeOut"] = printerConfig.inactivityTimeOut;
    json["controlChamberLight"] = printerConfig.controlChamberLight; //control chamber light
    // Debugging
    json["debugging"] = printerConfig.debugging;
    json["debugOnChange"] = printerConfig.debugOnChange;
    json["mqttdebug"] = printerConfig.mqttdebug;
    // Printer Dependant
    json["p1Printer"] = printerVariables.isP1Printer;
    json["doorSwitch"] = printerVariables.useDoorSwitch;
    // Customise LED Colors (during Lidar)
    json["stage14RGB"] = printerConfig.stage14Color.RGBhex;
    json["stage1RGB"] = printerConfig.stage1Color.RGBhex;
    json["stage8RGB"] = printerConfig.stage8Color.RGBhex;
    json["stage9RGB"] = printerConfig.stage9Color.RGBhex;
    json["stage10RGB"] = printerConfig.stage10Color.RGBhex;
    // Customise LED Colors
    json["errordetection"] = printerConfig.errordetection;
    json["wifiRGB"] = printerConfig.wifiRGB.RGBhex;
    json["pauseRGB"] = printerConfig.pauseRGB.RGBhex;
    json["firstlayerRGB"] = printerConfig.firstlayerRGB.RGBhex;
    json["nozzleclogRGB"] = printerConfig.nozzleclogRGB.RGBhex;
    json["hmsSeriousRGB"] = printerConfig.hmsSeriousRGB.RGBhex;
    json["hmsFatalRGB"] = printerConfig.hmsFatalRGB.RGBhex;
    json["filamentRunoutRGB"] = printerConfig.filamentRunoutRGB.RGBhex;
    json["frontCoverRGB"] = printerConfig.frontCoverRGB.RGBhex;
    json["nozzleTempRGB"] = printerConfig.nozzleTempRGB.RGBhex;
    json["bedTempRGB"] = printerConfig.bedTempRGB.RGBhex;
    //HMS Error handling
    json["hmsIgnoreList"] = printerConfig.hmsIgnoreList;

    // LED Hardware Configuration
    json["ledChipType"] = printerConfig.ledConfig.chipType;
    json["ledColorOrder"] = printerConfig.ledConfig.colorOrder;
    json["ledCount"] = printerConfig.ledConfig.ledCount;
    json["ledDataPin"] = printerConfig.ledConfig.dataPin;
    json["ledClockPin"] = printerConfig.ledConfig.clockPin;
    json["ledIsRGBW"] = printerConfig.ledConfig.isRGBW;

    // Pattern settings
    json["runningPattern"] = printerConfig.runningPattern;
    json["finishPattern"] = printerConfig.finishPattern;
    json["pausePattern"] = printerConfig.pausePattern;
    json["stage1Pattern"] = printerConfig.stage1Pattern;
    json["stage8Pattern"] = printerConfig.stage8Pattern;
    json["stage9Pattern"] = printerConfig.stage9Pattern;
    json["stage10Pattern"] = printerConfig.stage10Pattern;
    json["stage14Pattern"] = printerConfig.stage14Pattern;
    json["errorPattern"] = printerConfig.errorPattern;
    json["wifiPattern"] = printerConfig.wifiPattern;
    json["firstlayerPattern"] = printerConfig.firstlayerPattern;
    json["nozzleclogPattern"] = printerConfig.nozzleclogPattern;
    json["hmsSeriousPattern"] = printerConfig.hmsSeriousPattern;
    json["hmsFatalPattern"] = printerConfig.hmsFatalPattern;
    json["filamentRunoutPattern"] = printerConfig.filamentRunoutPattern;
    json["frontCoverPattern"] = printerConfig.frontCoverPattern;
    json["nozzleTempPattern"] = printerConfig.nozzleTempPattern;
    json["bedTempPattern"] = printerConfig.bedTempPattern;

    // Progress bar settings
    json["progressBarEnabled"] = printerConfig.progressBarEnabled;
    json["progressBgRGB"] = printerConfig.progressBarBackground.RGBhex;

    // Relay settings
    json["relayPin"] = printerConfig.relayPin;
    json["relayInverted"] = printerConfig.relayInverted;

    File configFile = LittleFS.open(configPath, "w");
    if (!configFile)
    {
        LogSerial.println(F("[Filesystem] Failed to save config"));
        return;
    }
    serializeJson(json, configFile);
    configFile.close();
    LogSerial.println(F("[Filesystem] Config Saved"));
}

void loadFileSystem()
{
    LogSerial.println(F("[Filesystem] Loading config"));

    File configFile;
    int attempts = 0;
    while (attempts < 2)
    {
        configFile = LittleFS.open(configPath, "r");
        if (configFile)
        {
            break;
        }
        attempts++;
        LogSerial.println(F("[Filesystem] Failed to open config file, retrying.."));
        delay(2000);
    }
    if (!configFile)
    {
        LogSerial.print(F("[Filesystem] Failed to open config file after "));
        LogSerial.print(attempts);
        LogSerial.println(F(" retries"));

        LogSerial.println(F("[Filesystem] Clearing config"));
        saveFileSystem();
        return;
    }
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    JsonDocument json;
    auto deserializeError = deserializeJson(json, buf.get());

    if (!deserializeError)
    {
        strlcpy(globalVariables.SSID, json["ssid"] | "", sizeof(globalVariables.SSID));
        strlcpy(globalVariables.APPW, json["appw"] | "", sizeof(globalVariables.APPW));
        strlcpy(securityVariables.HTTPUser, json["HTTPUser"] | "", sizeof(securityVariables.HTTPUser));
        strlcpy(securityVariables.HTTPPass, json["HTTPPass"] | "", sizeof(securityVariables.HTTPPass));
        strlcpy(printerConfig.printerIP, json["printerIp"] | "", sizeof(printerConfig.printerIP));
        strlcpy(printerConfig.accessCode, json["accessCode"] | "", sizeof(printerConfig.accessCode));
        strlcpy(printerConfig.serialNumber, json["serialNumber"] | "", sizeof(printerConfig.serialNumber));
        strlcpy(printerConfig.BSSID, json["bssi"] | "", sizeof(printerConfig.BSSID));
        printerConfig.brightness = json["brightness"];
        // LED Behaviour (Choose One)
        printerConfig.maintMode = json["maintMode"];
        printerConfig.discoMode = json["discoMode"];
        printerConfig.replicatestate = json["replicatestate"];
        // Running Color
        printerConfig.runningColor = hex2rgb(json["runningRGB"] | "#FFFFFF");
        // Test LED Colors
        printerConfig.testcolorEnabled = json["showtestcolor"];
        printerConfig.testColor = hex2rgb(json["testRGB"] | "#FFFFFF");
        printerConfig.debugwifi = json["debugwifi"];
        // Options
        printerConfig.finishIndication = json["finishIndication"];
        printerConfig.finishColor = hex2rgb(json["finishColor"] | "#00FF00");
        printerConfig.finishExit = json["finishExit"];
        printerConfig.finishTimeOut = json["finishTimerMins"];
        printerConfig.finish_check = json["finish_check"];
        printerConfig.inactivityEnabled = json["inactivityEnabled"];
        printerConfig.inactivityTimeOut = json["inactivityTimeOut"];
        printerConfig.controlChamberLight = json["controlChamberLight"]; //control chamber light
        // Debugging
        printerConfig.debugging = json["debugging"];
        printerConfig.debugOnChange = json["debugOnChange"];
        printerConfig.mqttdebug = json["mqttdebug"];
        // Printer Dependant
        printerVariables.isP1Printer = json["p1Printer"];
        printerVariables.useDoorSwitch = json["doorSwitch"];
        printerConfig.stage14Color = hex2rgb(json["stage14RGB"] | "#000000");
        printerConfig.stage1Color = hex2rgb(json["stage1RGB"] | "#000000");
        printerConfig.stage8Color = hex2rgb(json["stage8RGB"] | "#000000");
        printerConfig.stage9Color = hex2rgb(json["stage9RGB"] | "#000000");
        printerConfig.stage10Color = hex2rgb(json["stage10RGB"] | "#000000");
        // Customise LED Colors
        printerConfig.errordetection = json["errordetection"];
        printerConfig.wifiRGB = hex2rgb(json["wifiRGB"] | "#FFFFFF");

        printerConfig.pauseRGB = hex2rgb(json["pauseRGB"] | "#0000FF");
        printerConfig.firstlayerRGB = hex2rgb(json["firstlayerRGB"] | "#0000FF");
        printerConfig.nozzleclogRGB = hex2rgb(json["nozzleclogRGB"] | "#0000FF");
        printerConfig.hmsSeriousRGB = hex2rgb(json["hmsSeriousRGB"] | "#FF0000");
        printerConfig.hmsFatalRGB = hex2rgb(json["hmsFatalRGB"] | "#FF0000");
        printerConfig.filamentRunoutRGB = hex2rgb(json["filamentRunoutRGB"] | "#FF0000");
        printerConfig.frontCoverRGB = hex2rgb(json["frontCoverRGB"] | "#FF0000");
        printerConfig.nozzleTempRGB = hex2rgb(json["nozzleTempRGB"] | "#FF0000");
        printerConfig.bedTempRGB = hex2rgb(json["bedTempRGB"] | "#FF0000");
        // HMS Error handling
        printerConfig.hmsIgnoreList = json["hmsIgnoreList"] | "";

        // LED Hardware Configuration (with defaults for migration)
        printerConfig.ledConfig.chipType = json["ledChipType"] | CHIP_WS2812B;
        printerConfig.ledConfig.colorOrder = json["ledColorOrder"] | ORDER_GRB;
        printerConfig.ledConfig.ledCount = json["ledCount"] | 30;
        printerConfig.ledConfig.dataPin = json["ledDataPin"] | 16;
        printerConfig.ledConfig.clockPin = json["ledClockPin"] | 0;
        printerConfig.ledConfig.isRGBW = json["ledIsRGBW"] | false;

        // Pattern settings (with defaults for migration)
        printerConfig.runningPattern = json["runningPattern"] | PATTERN_SOLID;
        printerConfig.finishPattern = json["finishPattern"] | PATTERN_BREATHING;
        printerConfig.pausePattern = json["pausePattern"] | PATTERN_BREATHING;
        printerConfig.stage1Pattern = json["stage1Pattern"] | PATTERN_SOLID;
        printerConfig.stage8Pattern = json["stage8Pattern"] | PATTERN_SOLID;
        printerConfig.stage9Pattern = json["stage9Pattern"] | PATTERN_SOLID;
        printerConfig.stage10Pattern = json["stage10Pattern"] | PATTERN_CHASE;
        printerConfig.stage14Pattern = json["stage14Pattern"] | PATTERN_SOLID;
        printerConfig.errorPattern = json["errorPattern"] | PATTERN_BREATHING;
        printerConfig.wifiPattern = json["wifiPattern"] | PATTERN_SOLID;
        printerConfig.firstlayerPattern = json["firstlayerPattern"] | PATTERN_SOLID;
        printerConfig.nozzleclogPattern = json["nozzleclogPattern"] | PATTERN_BREATHING;
        printerConfig.hmsSeriousPattern = json["hmsSeriousPattern"] | PATTERN_BREATHING;
        printerConfig.hmsFatalPattern = json["hmsFatalPattern"] | PATTERN_BREATHING;
        printerConfig.filamentRunoutPattern = json["filamentRunoutPattern"] | PATTERN_BREATHING;
        printerConfig.frontCoverPattern = json["frontCoverPattern"] | PATTERN_SOLID;
        printerConfig.nozzleTempPattern = json["nozzleTempPattern"] | PATTERN_BREATHING;
        printerConfig.bedTempPattern = json["bedTempPattern"] | PATTERN_BREATHING;

        // Progress bar settings
        printerConfig.progressBarEnabled = json["progressBarEnabled"] | true;
        printerConfig.progressBarBackground = hex2rgb(json["progressBgRGB"] | "#000000");

        // Relay settings (with defaults for migration)
        printerConfig.relayPin = json["relayPin"] | -1;
        printerConfig.relayInverted = json["relayInverted"] | false;

        LogSerial.println(F("[Filesystem] Loaded config"));
    }
    else
    {
        LogSerial.println(F("[Filesystem] Failed loading config"));
        LogSerial.println(F("[Filesystem] Clearing config"));
        LittleFS.remove(configPath);
    }

    configFile.close();
}

void deleteFileSystem()
{
    LogSerial.println(F("[Filesystem] Deleting LittleFS"));
    LittleFS.remove(configPath);
}

bool hasFileSystem()
{
    return LittleFS.exists(configPath);
}

void setupFileSystem()
{
    LogSerial.println(F("[Filesystem] Mounting LittleFS"));
    if (!LittleFS.begin())
    {
        LogSerial.println(F("[Filesystem] Failed to mount LittleFS"));
        LittleFS.format();
        LogSerial.println(F("[Filesystem] Formatting LittleFS"));
        LogSerial.println(F("[Filesystem] Restarting Device"));
        delay(1000);
        ESP.restart();
    }
    LogSerial.println(F("[Filesystem] Mounted LittleFS"));
}

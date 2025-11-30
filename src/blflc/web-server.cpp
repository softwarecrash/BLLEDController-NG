#include "web-server.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <LittleFS.h>
#include "leds.h"
#include "filesystem.h"
#include "types.h"
#include "logserial.h"
#include "bblprinterdiscovery.h"

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

#include "../www/www.h"

unsigned long lastWsPush = 0;
const unsigned long wsPushInterval = 1000; // alle 1000ms

// External variables from main.cpp
extern bool shouldRestart;
extern unsigned long restartRequestTime;

bool isAuthorized(AsyncWebServerRequest *request)
{
    if (strlen(securityVariables.HTTPUser) == 0 || strlen(securityVariables.HTTPPass) == 0)
    {
        return true;
    }
    return request->authenticate(securityVariables.HTTPUser, securityVariables.HTTPPass);
}

void handleSetup(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, setupPage_html_gz_mime, setupPage_html_gz, setupPage_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleUpdatePage(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, updatePage_html_gz_mime, updatePage_html_gz, updatePage_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleGetIcon(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, blflc_svg_gz_mime, blflc_svg_gz, blflc_svg_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleGetfavicon(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, favicon_png_gz_mime, favicon_png_gz, favicon_png_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleGetPCC(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, particleCanvas_js_gz_mime, (const uint8_t *)particleCanvas_js_gz, particleCanvas_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleGetConfig(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }

    JsonDocument doc;

    doc["firmwareversion"] = globalVariables.FWVersion.c_str();
    doc["wifiStrength"] = WiFi.RSSI();
    doc["ip"] = printerConfig.printerIP;
    doc["code"] = printerConfig.accessCode;
    doc["id"] = printerConfig.serialNumber;
    doc["apMAC"] = printerConfig.BSSID;
    doc["brightness"] = printerConfig.brightness;

    // LED Hardware Configuration
    doc["ledChipType"] = printerConfig.ledConfig.chipType;
    doc["ledColorOrder"] = printerConfig.ledConfig.colorOrder;
    doc["ledCount"] = printerConfig.ledConfig.ledCount;
    doc["ledDataPin"] = printerConfig.ledConfig.dataPin;
    doc["progressBarEnabled"] = printerConfig.progressBarEnabled;
    doc["progressBgRGB"] = printerConfig.progressBarBackground.RGBhex;

    // LED Behaviour
    doc["maintMode"] = printerConfig.maintMode;
    doc["discoMode"] = printerConfig.discoMode;
    doc["replicateled"] = printerConfig.replicatestate;
    doc["runningRGB"] = printerConfig.runningColor.RGBhex;
    doc["runningPattern"] = printerConfig.runningPattern;
    doc["showtestcolor"] = printerConfig.testcolorEnabled;
    doc["testRGB"] = printerConfig.testColor.RGBhex;
    doc["debugwifi"] = printerConfig.debugwifi;
    doc["finishindication"] = printerConfig.finishIndication;
    doc["finishColor"] = printerConfig.finishColor.RGBhex;
    doc["finishPattern"] = printerConfig.finishPattern;
    doc["finishExit"] = printerConfig.finishExit;
    doc["finishTimerMins"] = (int)(printerConfig.finishTimeOut / 60000);
    doc["inactivityEnabled"] = printerConfig.inactivityEnabled;
    doc["inactivityMins"] = (int)(printerConfig.inactivityTimeOut / 60000);
    doc["debugging"] = printerConfig.debugging;
    doc["debugOnChange"] = printerConfig.debugOnChange;
    doc["mqttdebug"] = printerConfig.mqttdebug;
    doc["p1Printer"] = printerVariables.isP1Printer;
    doc["doorSwitch"] = printerVariables.useDoorSwitch;

    // Stage colors and patterns
    doc["stage14RGB"] = printerConfig.stage14Color.RGBhex;
    doc["stage14Pattern"] = printerConfig.stage14Pattern;
    doc["stage1RGB"] = printerConfig.stage1Color.RGBhex;
    doc["stage1Pattern"] = printerConfig.stage1Pattern;
    doc["stage8RGB"] = printerConfig.stage8Color.RGBhex;
    doc["stage8Pattern"] = printerConfig.stage8Pattern;
    doc["stage9RGB"] = printerConfig.stage9Color.RGBhex;
    doc["stage9Pattern"] = printerConfig.stage9Pattern;
    doc["stage10RGB"] = printerConfig.stage10Color.RGBhex;
    doc["stage10Pattern"] = printerConfig.stage10Pattern;

    // Error detection and colors
    doc["errordetection"] = printerConfig.errordetection;
    doc["wifiRGB"] = printerConfig.wifiRGB.RGBhex;
    doc["wifiPattern"] = printerConfig.wifiPattern;
    doc["pauseRGB"] = printerConfig.pauseRGB.RGBhex;
    doc["pausePattern"] = printerConfig.pausePattern;
    doc["firstlayerRGB"] = printerConfig.firstlayerRGB.RGBhex;
    doc["firstlayerPattern"] = printerConfig.firstlayerPattern;
    doc["nozzleclogRGB"] = printerConfig.nozzleclogRGB.RGBhex;
    doc["nozzleclogPattern"] = printerConfig.nozzleclogPattern;
    doc["hmsSeriousRGB"] = printerConfig.hmsSeriousRGB.RGBhex;
    doc["hmsSeriousPattern"] = printerConfig.hmsSeriousPattern;
    doc["hmsFatalRGB"] = printerConfig.hmsFatalRGB.RGBhex;
    doc["hmsFatalPattern"] = printerConfig.hmsFatalPattern;
    doc["filamentRunoutRGB"] = printerConfig.filamentRunoutRGB.RGBhex;
    doc["filamentRunoutPattern"] = printerConfig.filamentRunoutPattern;
    doc["frontCoverRGB"] = printerConfig.frontCoverRGB.RGBhex;
    doc["frontCoverPattern"] = printerConfig.frontCoverPattern;
    doc["nozzleTempRGB"] = printerConfig.nozzleTempRGB.RGBhex;
    doc["nozzleTempPattern"] = printerConfig.nozzleTempPattern;
    doc["bedTempRGB"] = printerConfig.bedTempRGB.RGBhex;
    doc["bedTempPattern"] = printerConfig.bedTempPattern;

    // HMS Error Handling
    doc["hmsIgnoreList"] = printerConfig.hmsIgnoreList;
    // control chamber light
    doc["controlChamberLight"] = printerConfig.controlChamberLight;

    // Relay settings
    doc["relayPin"] = printerConfig.relayPin;
    doc["relayInverted"] = printerConfig.relayInverted;

    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, "application/json", jsonString);
}

void handlePrinterConfigJson(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    JsonDocument doc;
    doc["ssid"] = globalVariables.SSID;
    doc["pass"] = globalVariables.APPW;
    doc["printerIP"] = printerConfig.printerIP;
    doc["printerSerial"] = printerConfig.serialNumber;
    doc["accessCode"] = printerConfig.accessCode;
    doc["webUser"] = securityVariables.HTTPUser;
    doc["webPass"] = securityVariables.HTTPPass;
    doc["isAPMode"] = (WiFi.getMode() & WIFI_AP);

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleStyleCss(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, style_css_gz_mime, style_css_gz, style_css_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleSubmitConfig(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }

    auto getSafeParamValue = [](AsyncWebServerRequest *req, const char *name, const char *fallback = "") -> String
    {
        return req->hasParam(name, true) ? req->getParam(name, true)->value() : fallback;
    };

    auto getSafeParamInt = [](AsyncWebServerRequest *req, const char *name, int fallback = 0) -> int
    {
        return req->hasParam(name, true) ? req->getParam(name, true)->value().toInt() : fallback;
    };

    // Check if LED hardware config changed (requires reinit)
    uint8_t oldChipType = printerConfig.ledConfig.chipType;
    uint8_t oldDataPin = printerConfig.ledConfig.dataPin;
    uint16_t oldLedCount = printerConfig.ledConfig.ledCount;
    uint8_t oldColorOrder = printerConfig.ledConfig.colorOrder;

    printerConfig.brightness = getSafeParamInt(request, "brightnessslider");
    printerConfig.rescanWiFiNetwork = request->hasParam("rescanWiFiNetwork", true);
    printerConfig.maintMode = request->hasParam("maintMode", true);
    printerConfig.discoMode = request->hasParam("discoMode", true);
    printerConfig.replicatestate = request->hasParam("replicateLedState", true);
    printerConfig.runningColor = hex2rgb(getSafeParamValue(request, "runningRGB", "#FFFFFF"));
    printerConfig.testcolorEnabled = request->hasParam("showtestcolor", true);
    printerConfig.testColor = hex2rgb(getSafeParamValue(request, "testRGB", "#FFFFFF"));
    printerConfig.debugwifi = request->hasParam("debugwifi", true);
    printerConfig.finishIndication = request->hasParam("finishIndication", true);
    printerConfig.finishColor = hex2rgb(getSafeParamValue(request, "finishColor", "#00FF00"));
    printerConfig.finishExit = !request->hasParam("finishEndTimer", true);
    printerConfig.finishTimeOut = getSafeParamInt(request, "finishTimerMins") * 60000;
    printerConfig.inactivityEnabled = request->hasParam("inactivityEnabled", true);
    printerConfig.inactivityTimeOut = getSafeParamInt(request, "inactivityMins") * 60000;
    printerConfig.debugging = request->hasParam("debugging", true);
    printerConfig.debugOnChange = request->hasParam("debugOnChange", true);
    printerConfig.mqttdebug = request->hasParam("mqttdebug", true);
    printerVariables.isP1Printer = request->hasParam("p1Printer", true);
    printerVariables.useDoorSwitch = request->hasParam("doorSwitch", true);

    printerConfig.stage14Color = hex2rgb(getSafeParamValue(request, "stage14RGB", "#000000"));
    printerConfig.stage1Color = hex2rgb(getSafeParamValue(request, "stage1RGB", "#000000"));
    printerConfig.stage8Color = hex2rgb(getSafeParamValue(request, "stage8RGB", "#000000"));
    printerConfig.stage9Color = hex2rgb(getSafeParamValue(request, "stage9RGB", "#000000"));
    printerConfig.stage10Color = hex2rgb(getSafeParamValue(request, "stage10RGB", "#000000"));
    printerConfig.errordetection = request->hasParam("errorDetection", true);
    printerConfig.wifiRGB = hex2rgb(getSafeParamValue(request, "wifiRGB", "#FFFFFF"));
    printerConfig.pauseRGB = hex2rgb(getSafeParamValue(request, "pauseRGB", "#0000FF"));
    printerConfig.firstlayerRGB = hex2rgb(getSafeParamValue(request, "firstlayerRGB", "#0000FF"));
    printerConfig.nozzleclogRGB = hex2rgb(getSafeParamValue(request, "nozzleclogRGB", "#0000FF"));
    printerConfig.hmsSeriousRGB = hex2rgb(getSafeParamValue(request, "hmsSeriousRGB", "#FF0000"));
    printerConfig.hmsFatalRGB = hex2rgb(getSafeParamValue(request, "hmsFatalRGB", "#FF0000"));
    printerConfig.filamentRunoutRGB = hex2rgb(getSafeParamValue(request, "filamentRunoutRGB", "#FF0000"));
    printerConfig.frontCoverRGB = hex2rgb(getSafeParamValue(request, "frontCoverRGB", "#FF0000"));
    printerConfig.nozzleTempRGB = hex2rgb(getSafeParamValue(request, "nozzleTempRGB", "#FF0000"));
    printerConfig.bedTempRGB = hex2rgb(getSafeParamValue(request, "bedTempRGB", "#FF0000"));
    // HMS Error handling
    printerConfig.hmsIgnoreList = getSafeParamValue(request, "hmsIgnoreList");
    // Control Chamber Light
    printerConfig.controlChamberLight = request->hasParam("controlChamberLight", true);

    // LED Hardware Configuration
    printerConfig.ledConfig.chipType = getSafeParamInt(request, "ledChipType", CHIP_WS2812B);
    printerConfig.ledConfig.colorOrder = getSafeParamInt(request, "ledColorOrder", ORDER_GRB);
    printerConfig.ledConfig.ledCount = constrain(getSafeParamInt(request, "ledCount", 30), 1, 300);
    printerConfig.ledConfig.dataPin = getSafeParamInt(request, "ledDataPin", 16);
    printerConfig.ledConfig.clockPin = getSafeParamInt(request, "ledClockPin", 0);

    // Pattern settings
    printerConfig.runningPattern = getSafeParamInt(request, "runningPattern", PATTERN_SOLID);
    printerConfig.finishPattern = getSafeParamInt(request, "finishPattern", PATTERN_BREATHING);
    printerConfig.pausePattern = getSafeParamInt(request, "pausePattern", PATTERN_BREATHING);
    printerConfig.stage1Pattern = getSafeParamInt(request, "stage1Pattern", PATTERN_SOLID);
    printerConfig.stage8Pattern = getSafeParamInt(request, "stage8Pattern", PATTERN_SOLID);
    printerConfig.stage9Pattern = getSafeParamInt(request, "stage9Pattern", PATTERN_SOLID);
    printerConfig.stage10Pattern = getSafeParamInt(request, "stage10Pattern", PATTERN_CHASE);
    printerConfig.stage14Pattern = getSafeParamInt(request, "stage14Pattern", PATTERN_SOLID);
    printerConfig.errorPattern = getSafeParamInt(request, "errorPattern", PATTERN_BREATHING);
    // Error type specific patterns
    printerConfig.wifiPattern = getSafeParamInt(request, "wifiPattern", PATTERN_SOLID);
    printerConfig.firstlayerPattern = getSafeParamInt(request, "firstlayerPattern", PATTERN_SOLID);
    printerConfig.nozzleclogPattern = getSafeParamInt(request, "nozzleclogPattern", PATTERN_BREATHING);
    printerConfig.hmsSeriousPattern = getSafeParamInt(request, "hmsSeriousPattern", PATTERN_BREATHING);
    printerConfig.hmsFatalPattern = getSafeParamInt(request, "hmsFatalPattern", PATTERN_BREATHING);
    printerConfig.filamentRunoutPattern = getSafeParamInt(request, "filamentRunoutPattern", PATTERN_BREATHING);
    printerConfig.frontCoverPattern = getSafeParamInt(request, "frontCoverPattern", PATTERN_SOLID);
    printerConfig.nozzleTempPattern = getSafeParamInt(request, "nozzleTempPattern", PATTERN_BREATHING);
    printerConfig.bedTempPattern = getSafeParamInt(request, "bedTempPattern", PATTERN_BREATHING);

    // Progress bar settings
    printerConfig.progressBarEnabled = request->hasParam("progressBarEnabled", false);
    printerConfig.progressBarBackground = hex2rgb(getSafeParamValue(request, "progressBgRGB", "#000000"));

    // Relay settings
    int8_t oldRelayPin = printerConfig.relayPin;
    bool oldRelayInverted = printerConfig.relayInverted;

    printerConfig.relayPin = getSafeParamInt(request, "relayPin", -1);
    printerConfig.relayInverted = request->hasParam("relayInverted", true);

    saveFileSystem();
    LogSerial.println(F("Packet received from setuppage"));

    // Reinitialize relay if pin changed
    if (printerConfig.relayPin != oldRelayPin || printerConfig.relayInverted != oldRelayInverted)
    {
        LogSerial.println(F("[Relay] Configuration changed, reinitializing..."));
        setupRelay();
    }

    // Reinitialize LEDs if hardware config changed
    if (printerConfig.ledConfig.chipType != oldChipType ||
        printerConfig.ledConfig.dataPin != oldDataPin ||
        printerConfig.ledConfig.ledCount != oldLedCount ||
        printerConfig.ledConfig.colorOrder != oldColorOrder)
    {
        LogSerial.println(F("[LED] Hardware config changed, reinitializing..."));
        setupLeds();
    }


    printerConfig.inactivityStartms = millis();
    printerConfig.isIdleOFFActive = false;
    printerConfig.replicate_update = true;
    printerConfig.maintMode_update = true;
    printerConfig.discoMode_update = true;
    printerConfig.testcolor_update = true;
    updateleds();
    request->send(200, "text/plain", "OK");
}

// LED Test endpoint
void handleLedTest(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }
    printerConfig.ledTestMode = true;
    LogSerial.println(F("[LED] Starting test sequence"));
    request->send(200, "application/json", "{\"status\":\"testing\"}");
}

void sendJsonToAll(JsonDocument &doc)
{
    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);
}

void handleWiFiScan(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i)
    {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["bssid"] = WiFi.BSSIDstr(i);
        net["rssi"] = WiFi.RSSI(i);
        net["enc"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? false : true;
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleWiFiSetupPage(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(200, wifiSetup_html_gz_mime, wifiSetup_html_gz, wifiSetup_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleSubmitWiFi(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }

    // Check for SSID + PASS optional handling
    bool hasSSID = request->hasParam("ssid", true);
    bool hasPASS = request->hasParam("pass", true);

    if (hasSSID && hasPASS)
    {
        String ssid = request->getParam("ssid", true)->value();
        String pass = request->getParam("pass", true)->value();
        String bssid = request->hasParam("bssid", true) ? request->getParam("bssid", true)->value() : "";
        if (bssid.length() > 0)
            strlcpy(printerConfig.BSSID, bssid.c_str(), sizeof(printerConfig.BSSID));

        ssid.trim();
        pass.trim();

        if (ssid.length() > 0 && pass.length() > 0)
        {
            LogSerial.println(F("[WiFiSetup] Updating WiFi credentials:"));
            LogSerial.print(F("SSID: "));
            LogSerial.println(ssid);
            LogSerial.println(F("Password: ********"));

            strlcpy(globalVariables.SSID, ssid.c_str(), sizeof(globalVariables.SSID));
            strlcpy(globalVariables.APPW, pass.c_str(), sizeof(globalVariables.APPW));
        }
        else
        {
            LogSerial.println(F("[WiFiSetup] Empty SSID or PASS received → ignoring WiFi update."));
        }
    }
    else
    {
        LogSerial.println(F("[WiFiSetup] No SSID or PASS provided → keeping existing WiFi credentials."));
    }

    // Optional other fields (printerIP, printerSerial, etc.)
    String printerIP = request->hasParam("printerIP", true) ? request->getParam("printerIP", true)->value() : "";
    String printerSerial = request->hasParam("printerSerial", true) ? request->getParam("printerSerial", true)->value() : "";
    String accessCode = request->hasParam("accessCode", true) ? request->getParam("accessCode", true)->value() : "";
    String webUser = request->hasParam("webUser", true) ? request->getParam("webUser", true)->value() : "";
    String webPass = request->hasParam("webPass", true) ? request->getParam("webPass", true)->value() : "";

    if (printerIP.length() > 0)
        strlcpy(printerConfig.printerIP, printerIP.c_str(), sizeof(printerConfig.printerIP));
    if (printerSerial.length() > 0)
        strlcpy(printerConfig.serialNumber, printerSerial.c_str(), sizeof(printerConfig.serialNumber));
    if (accessCode.length() > 0)
        strlcpy(printerConfig.accessCode, accessCode.c_str(), sizeof(printerConfig.accessCode));

    strlcpy(securityVariables.HTTPUser, webUser.c_str(), sizeof(securityVariables.HTTPUser));
    strlcpy(securityVariables.HTTPPass, webPass.c_str(), sizeof(securityVariables.HTTPPass));

    saveFileSystem();

    request->send(200, "text/plain", "Settings saved, restarting...");
    shouldRestart = true;
    restartRequestTime = millis();
}

void websocketLoop()
{
    if (ws.count() == 0)
        return;
    if (millis() - lastWsPush > wsPushInterval)
    {
        lastWsPush = millis();

        JsonDocument doc;
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["ip"] = WiFi.localIP().toString();
        doc["uptime"] = millis() / 1000;
        doc["doorOpen"] = printerVariables.doorOpen;
        doc["printerConnection"] = printerVariables.online;
        doc["clients"] = ws.count();
        doc["stg_cur"] = printerVariables.stage;
        sendJsonToAll(doc);
    }
}

void handleConfigPage(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
        return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(200, backupRestore_html_gz_mime, backupRestore_html_gz, backupRestore_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handleDownloadConfigFile(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
    {
        return request->requestAuthentication();
    }

    if (!LittleFS.exists(configPath))
    {
        request->send(404, "text/plain", "Config file not found");
        return;
    }

    File configFile = LittleFS.open(configPath, "r");
    if (!configFile)
    {
        request->send(500, "text/plain", "Failed to open config file");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        request->send(500, "text/plain", "Failed to parse config file");
        return;
    }

    String jsonString;
    serializeJsonPretty(doc, jsonString);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonString);
    response->addHeader("Content-Disposition", "attachment; filename=\"blflcconfig.json\"");
    request->send(response);
}

void handleWebSerialPage(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
        return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(200, webSerialPage_html_gz_mime, webSerialPage_html_gz, webSerialPage_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void handlePrinterList(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < bblKnownPrinterCount; i++)
    {
        JsonObject obj = arr.add<JsonObject>();
        obj["ip"] = bblLastKnownPrinters[i].ip.toString();
        obj["usn"] = bblLastKnownPrinters[i].usn;
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleFactoryReset(AsyncWebServerRequest *request)
{
    if (!isAuthorized(request))
        return request->requestAuthentication();

    LogSerial.println(F("[FactoryReset] Performing full reset..."));

    deleteFileSystem(); // delete LittleFS config
    request->send(200, "text/plain", "Factory reset complete. Restarting...");

    shouldRestart = true;
    restartRequestTime = millis();
}

void handleUploadConfigFileData(AsyncWebServerRequest *request, const String &filename,
                                size_t index, uint8_t *data, size_t len, bool final)
{
    static File uploadFile;

    if (!index)
    {
        LogSerial.println(F("[ConfigUpload] Start"));
        uploadFile = LittleFS.open(configPath, "w");
    }
    if (uploadFile)
    {
        uploadFile.write(data, len);
    }
    if (final)
    {
        uploadFile.close();
        LogSerial.println(F("[ConfigUpload] Finished"));
    }
    shouldRestart = true;
    restartRequestTime = millis();
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        LogSerial.printf("[WS] Client connected: %u\n", client->id());
        websocketLoop();
        break;
    case WS_EVT_DISCONNECT:
        LogSerial.printf("[WS] Client disconnected: %u\n", client->id());
        ws.cleanupClients();
        break;
    case WS_EVT_DATA:
        LogSerial.printf("[WS] Data received from client %u\n", client->id());
        break;
    case WS_EVT_PONG:
        LogSerial.printf("[WS] Pong received from %u\n", client->id());
        break;
    case WS_EVT_ERROR:
        LogSerial.printf("[WS] Error on connection %u\n", client->id());
        ws.cleanupClients();
        break;
    }
}

void setupWebserver()
{
    if (!MDNS.begin(globalVariables.Host.c_str()))
    {
        LogSerial.println(F("Error setting up MDNS responder!"));
        while (1)
            delay(500);
    }

    LogSerial.println(F("Setting up webserver"));

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    if (WiFi.getMode() == WIFI_AP) {
        LogSerial.println(F("[WebServer] Captive Portal activ – redirect to /wifi"));
        request->redirect("/wifi");
    } else {
        handleSetup(request);
    } });
    webServer.on("/fwupdate", HTTP_GET, handleUpdatePage);
    webServer.on("/getConfig", HTTP_GET, handleGetConfig);
    webServer.on("/submitConfig", HTTP_POST, handleSubmitConfig);
    webServer.on("/blflc.svg", HTTP_GET, handleGetIcon);
    webServer.on("/favicon.ico", HTTP_GET, handleGetfavicon);
    webServer.on("/particleCanvas.js", HTTP_GET, handleGetPCC);
    webServer.on("/config.json", HTTP_GET, handlePrinterConfigJson);
    webServer.on("/wifi", HTTP_GET, handleWiFiSetupPage);
    webServer.on("/wifiScan", HTTP_GET, handleWiFiScan);
    webServer.on("/submitWiFi", HTTP_POST, handleSubmitWiFi);
    webServer.on("/style.css", HTTP_GET, handleStyleCss);
    webServer.on("/backuprestore", HTTP_GET, handleConfigPage);
    webServer.on("/configfile.json", HTTP_GET, handleDownloadConfigFile);
    webServer.on("/webserial", HTTP_GET, handleWebSerialPage);
    webServer.on("/printerList", HTTP_GET, handlePrinterList);
    webServer.on("/factoryreset", HTTP_GET, handleFactoryReset);
    webServer.on("/api/ledtest", HTTP_POST, handleLedTest);
    webServer.on("/configrestore", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!isAuthorized(request)) {
            return request->requestAuthentication();
        }
        request->send(200, "text/plain", "Config uploaded. Restarting...");
        shouldRestart = true;
        restartRequestTime = millis(); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
                 {
        static File uploadFile;

        if (!index) {
            LogSerial.printf("[ConfigUpload] Start: %s\n", filename.c_str());
            uploadFile = LittleFS.open(configPath, "w");
        }
        if (uploadFile) {
            uploadFile.write(data, len);
        }
        if (final) {
            uploadFile.close();
            LogSerial.println(F("[ConfigUpload] Finished"));
        } });

    webServer.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        request->send(200, "text/plain", "OK");
        LogSerial.println(F("OTA Upload done. Marking for restart."));
        shouldRestart = true;
        restartRequestTime = millis(); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
                 {
        if (!index) {
            LogSerial.printf("[OTA] Start: %s\n", filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(LogSerial);
            }
        }

        if (Update.write(data, len) != len) {
            Update.printError(LogSerial);
        }

        if (final) {
            if (Update.end(true)) {
                LogSerial.printf("[OTA] Success (%u bytes). Awaiting reboot...\n", index + len);
            } else {
                Update.printError(LogSerial);
            }
        } });

    LogSerial.begin(&webServer);

    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

    webServer.begin();

    LogSerial.println(F("Webserver started"));
}

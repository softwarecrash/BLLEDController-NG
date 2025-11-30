#ifndef _BLFLCWEB_SERVER
#define _BLFLCWEB_SERVER

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Web server instance
extern AsyncWebServer webServer;
extern AsyncWebSocket ws;

// WebSocket timing
extern unsigned long lastWsPush;
extern const unsigned long wsPushInterval;

// Authorization check
bool isAuthorized(AsyncWebServerRequest *request);

// Page handlers
void handleSetup(AsyncWebServerRequest *request);
void handleUpdatePage(AsyncWebServerRequest *request);
void handleGetIcon(AsyncWebServerRequest *request);
void handleGetfavicon(AsyncWebServerRequest *request);
void handleGetPCC(AsyncWebServerRequest *request);
void handleGetConfig(AsyncWebServerRequest *request);
void handlePrinterConfigJson(AsyncWebServerRequest *request);
void handleStyleCss(AsyncWebServerRequest *request);
void handleSubmitConfig(AsyncWebServerRequest *request);
void handleLedTest(AsyncWebServerRequest *request);
void handleWiFiScan(AsyncWebServerRequest *request);
void handleWiFiSetupPage(AsyncWebServerRequest *request);
void handleSubmitWiFi(AsyncWebServerRequest *request);
void handleConfigPage(AsyncWebServerRequest *request);
void handleDownloadConfigFile(AsyncWebServerRequest *request);
void handleWebSerialPage(AsyncWebServerRequest *request);
void handlePrinterList(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);
void handleUploadConfigFileData(AsyncWebServerRequest *request, const String &filename,
                                size_t index, uint8_t *data, size_t len, bool final);

// WebSocket functions
void sendJsonToAll(JsonDocument &doc);
void websocketLoop();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len);

// Setup function
void setupWebserver();

#endif

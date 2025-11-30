#include "ssdp.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESP32SSDP.h>

void start_ssdp() {
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setDeviceType("urn:schemas-upnp-org:device:DimmableLight:1");
    SSDP.setName("BLFastLED Controller");
    SSDP.setSerialNumber(WiFi.macAddress().c_str());
    SSDP.setURL("/");
    SSDP.setModelName("BLFLC ESP32");
    SSDP.setModelNumber("3.0");
    SSDP.setManufacturer("Djelibeybi");
    SSDP.setManufacturerURL("https://djelibeybi.github.io/BLFastLEDController");
    SSDP.begin();
}

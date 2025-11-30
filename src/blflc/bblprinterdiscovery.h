#ifndef _BBLPRINTERDISCOVERY_H
#define _BBLPRINTERDISCOVERY_H

#include <WiFi.h>
#include <WiFiUdp.h>

#define BBL_SSDP_PORT 2021
#define BBL_SSDP_MCAST_IP IPAddress(239, 255, 255, 250)
#define BBL_DISCOVERY_INTERVAL 30000UL
#define BBL_SSDP_SEARCH_TIMEOUT_MS 5000UL
#define BBL_MAX_PRINTERS 10

struct BBLPrinter
{
    IPAddress ip;
    char usn[64];
};

extern BBLPrinter bblLastKnownPrinters[BBL_MAX_PRINTERS];
extern int bblKnownPrinterCount;

bool bblIsPrinterKnown(IPAddress ip, int *index = nullptr);
void bblPrintKnownPrinters();
void bblSearchPrinters();

#endif

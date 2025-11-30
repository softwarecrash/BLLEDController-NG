#ifndef _TYPES
#define _TYPES

#ifdef __cplusplus
extern "C"
{
#endif

    // LED strip types supported by FastLED
    enum LedChipType {
        CHIP_WS2812B = 0,
        CHIP_SK6812 = 1,
        CHIP_SK6812_RGBW = 2,
        CHIP_APA102 = 3,
        CHIP_WS2811 = 4,
        CHIP_NEOPIXEL = 5,
        CHIP_WS2814_RGBW = 6
    };;

    // Color order options
    enum LedColorOrder {
        ORDER_GRB = 0,   // WS2812B default
        ORDER_RGB = 1,
        ORDER_BRG = 2,
        ORDER_RBG = 3,
        ORDER_BGR = 4,
        ORDER_GBR = 5,
        ORDER_GRBW = 6,  // SK6812 RGBW
        ORDER_RGBW = 7,
        ORDER_RWGB = 8   // WS2814 RGBW
    };;

    // LED pattern effects
    enum LedPattern {
        PATTERN_SOLID = 0,      // All LEDs same color
        PATTERN_BREATHING = 1,  // Brightness pulsing
        PATTERN_CHASE = 2,      // Moving light
        PATTERN_RAINBOW = 3,    // Color cycle
        PATTERN_PROGRESS = 4    // Print progress bar
    };

    typedef struct COLORStruct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        char RGBhex[8];
    } COLOR;

    // LED hardware configuration
    typedef struct LedConfigStruct {
        uint8_t chipType = CHIP_WS2812B;
        uint8_t colorOrder = ORDER_GRB;
        uint16_t ledCount = 30;
        uint8_t dataPin = 16;
        uint8_t clockPin = 0;  // For APA102 only
        bool isRGBW = false;
    } LedConfig;


    typedef struct PrinterVariablesStruct{
        String parsedHMSlevel = "";
        uint64_t parsedHMScode = 0;            //8 bytes per code stored
        String gcodeState = "FINISH";           //Initialised to Finish so the logic doesn't
                                                //assume a Print has just finished and needs
                                                //to wait for a door interaction to continue
        int stage = 0;
        uint8_t printProgress = 0;             // Print progress percentage (0-100)
        int overridestage = 999;
        bool printerLedState = true;
        bool hmsstate = false;
        bool online = false;
        bool finished = false;
        bool initializedLEDs = false;
        //Time since
        unsigned long disconnectMQTTms = 0;

        //PrinterType
        bool isP1Printer = false;               //Is this a P1 Printer without lidar or door switch
        //Door Monitoring
        bool useDoorSwitch = true;              //DoorSwitch to be used for Actions?
        bool doorOpen = false;                  // Current State of Door
        bool doorSwitchTriggered = false;       // Has door been closed twice within 6 seconds?
        bool waitingForDoor = false;            // Are we waiting for the door to be actuated?
        unsigned long lastdoorClosems = 0;      // Last time door was opened
        unsigned long lastdoorOpenms = 0;       // Last time door was closed
        bool chamberLightLocked = false;  // blocks replicate while true
        bool ledWasForcedByDoor = false;
    } PrinterVariables;
    extern PrinterVariables printerVariables;

    typedef struct SecurityVariables{
                // Security
        char HTTPUser[40] = "";             //http basic auth username
        char HTTPPass[40] = "";              //http basic auth password
    }SecurityVariables;
    extern SecurityVariables securityVariables;



    typedef struct GlobalVariablesStruct{
        char SSID[32];
        char APPW[64];
        String FWVersion = STRVERSION;
        String Host = "BLFLC";
        bool started = false;
    } GlobalVariables;

    extern GlobalVariables globalVariables;

    typedef struct PrinterConfigStruct
    {
        char printerIP[16];             //BBLP IP Address - used for MQTT reports
        char accessCode[9];             //BBLP Access Code - used for MQTT reports
        char serialNumber[16];          //BBLP Serial Number - used for MQTT reports

        char BSSID[18];                 //Nominated AP to connect to (Useful if multiple accesspoints with same name)
        int brightness = 20;            //Brightness of LEDS - Default to 20% in case user use LED's that draw too much power for their PS
        bool rescanWiFiNetwork = false; //Scans available WiFi networks for strongest signal
        // LED Behaviour (Choose One)
        bool maintMode = false;         //White lights on, even if printer is unpowered
        bool maintMode_update = true;
        bool discoMode = false;         //Cycles through RGB colors slowly for 'pretty' timelapse movie
        bool discoMode_update = true;
        bool replicatestate = true;     //LED will be on if the BBPL Light is on
        bool replicate_update = true;     //LED will be on if the BBPL Light is on
        COLOR runningColor;             //Running Color (Default if no issues)
        bool testcolorEnabled = false;
        bool testcolor_update= true;    //When updateleds() is run, should the TEST LEDS be set?
        COLOR testColor;                //Test Color
        bool debugwifi = false;         //Changes LED to a color range that represents WiFi signal Strength
        // Options
        bool finishIndication = true;   //Enable / Disable
        COLOR finishColor;              //Set Finish Color
        bool finishExit = true;         //True = use Door / False = use Timer
        bool finish_check = false;    //When updateleds() is run, should the TEST LEDS be set?
        unsigned long finishStartms = 0;    // Time the finish countdown is measured from
        int finishTimeOut = 600000;     //300000 = 5 mins
        bool controlChamberLight = false;                //control chamber light

        //Inactivity Timout
        bool inactivityEnabled = true;
        bool isIdleOFFActive = false;       // Are the lights out due to inactivity Timeout?
        unsigned long inactivityStartms = 0;    // Time the inactivity countdown is measured from
        int inactivityTimeOut = 3600000;  // 1800000 = 30mins / 600000 = 10mins / 60000 = 1mins
        // Debugging
        bool debugging = false;          //Debugging for all interactions through functions
        bool debugOnChange = true;     //Default debugging level - to shows onChange
        bool mqttdebug = false;         //Writes each packet from BBLP to the serial log
        //Custom Colors for events using lidar
        COLOR stage14Color;
        COLOR stage1Color;
        COLOR stage8Color;
        COLOR stage9Color;
        COLOR stage10Color;
        // Customise LED Colors
        bool errordetection = true;     //Utilises Error Colors when BBLP give an error
        COLOR wifiRGB;
        COLOR pauseRGB;
        COLOR firstlayerRGB;
        COLOR nozzleclogRGB;
        COLOR hmsSeriousRGB;
        COLOR hmsFatalRGB;
        COLOR filamentRunoutRGB;
        COLOR frontCoverRGB;
        COLOR nozzleTempRGB;
        COLOR bedTempRGB;
        // HMS Error Handling
        String hmsIgnoreList; // comma-separated list of HMS_XXXX_XXXX_XXXX_XXXX codes to ignore

        // LED Hardware Configuration
        LedConfig ledConfig;

        // Stage patterns (in addition to colors)
        uint8_t runningPattern = PATTERN_SOLID;
        uint8_t finishPattern = PATTERN_BREATHING;
        uint8_t pausePattern = PATTERN_BREATHING;
        uint8_t stage1Pattern = PATTERN_SOLID;
        uint8_t stage8Pattern = PATTERN_SOLID;
        uint8_t stage9Pattern = PATTERN_SOLID;
        uint8_t stage10Pattern = PATTERN_CHASE;
        uint8_t stage14Pattern = PATTERN_SOLID;
        uint8_t errorPattern = PATTERN_BREATHING;
        uint8_t wifiPattern = PATTERN_SOLID;
        uint8_t firstlayerPattern = PATTERN_SOLID;
        uint8_t nozzleclogPattern = PATTERN_BREATHING;
        uint8_t hmsSeriousPattern = PATTERN_BREATHING;
        uint8_t hmsFatalPattern = PATTERN_BREATHING;
        uint8_t filamentRunoutPattern = PATTERN_BREATHING;
        uint8_t frontCoverPattern = PATTERN_SOLID;
        uint8_t nozzleTempPattern = PATTERN_BREATHING;
        uint8_t bedTempPattern = PATTERN_BREATHING;

        // Progress bar settings
        bool progressBarEnabled = true;
        COLOR progressBarBackground;  // Unlit portion color (default black)

        // LED test mode
        bool ledTestMode = false;

        // Relay control for LED power
        int8_t relayPin = -1;           // GPIO pin for relay (-1 = disabled)
        bool relayInverted = false;     // If true, relay is active HIGH; default is active LOW

    } PrinterConfig;

    extern PrinterConfig printerConfig;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
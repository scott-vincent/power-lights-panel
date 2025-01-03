#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <cstring>
#define _stricmp strcasecmp

class settings;
class simvars;
class gpioctrl;

enum Aircraft {
    UNDEFINED,
    NO_AIRCRAFT,
    CESSNA_152,
    CESSNA_172,
    CESSNA_CJ4,
    SAVAGE_CUB,
    SHOCK_ULTRA,
    AIRBUS_A310,
    FBW,
    BOEING_747,
    BOEING_787,
    SUPERMARINE_SPITFIRE,
    F15_EAGLE,
    F18_HORNET,
    JUSTFLIGHT_PA28,
    OTHER_AIRCRAFT,
    OTHER_AIRCRAFT2,
};

struct globalVars
{
    const char* Cessna_152_Text = "Cessna 152";
    const int Cessna_152_Len = 10;
    const char* Cessna_172_Text = "Cessna Skyhawk";
    const int Cessna_172_Len = 14;
    const char* Cessna_CJ4_Text = "Cessna CJ4";
    const int Cessna_CJ4_Len = 10;
    const char* Savage_Cub_Text = "Asobo Savage Cub";
    const int Savage_Cub_Len = 16;
    const char* Shock_Ultra_Text = "Savage Shock Ultra";
    const int Shock_Ultra_Len = 18;
    const char* Boeing_747_Text = "Boeing 747-8";
    const int Boeing_747_Len = 12;
    const char* Salty_Boeing_747_Text = "Salty Boeing 747";
    const int Salty_Boeing_747_Len = 16;
    const char* Boeing_787_Text = "Boeing 787";
    const int Boeing_787_Len = 10;
    const char* Supermarine_Spitfire_Text = "FlyingIron Spitfire";
    const int Supermarine_Spitfire_Len = 19;
    const char* F15_Eagle_Text = "DCD F-15";
    const int F15_Eagle_Len = 8;
    const char* F18_Hornet_Text = "Boeing F/A 18E";
    const int F18_Hornet_Len = 14;
    const char* JustFlight_PA28_Text = "Just Flight PA28";
    const int JustFlight_PA28_Len = 16;

    const int FastAircraftSpeed = 195;

    const char* BitmapDir = "bitmaps/";
    const char* SettingsDir = "settings/";
    const char* SettingsFile = "settings/power-lights-panel.json";

    settings* allSettings = NULL;
    simvars* simVars = NULL;
    gpioctrl* gpioCtrl = NULL;
    Aircraft aircraft;
    char lastAircraft[32];

    long dataRateFps = 8;
    bool quit = false;
    bool dataLinked = false;
    bool connected = false;
    bool electrics = false;
    bool avionics = false;
};

#endif // _GLOBALS_H_

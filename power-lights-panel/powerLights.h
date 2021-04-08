#ifndef _POWER_LIGHTS_H_
#define _POWER_LIGHTS_H_

#include "simvars.h"

class powerLights
{
private:
    SimVars* simVars;
    Aircraft loadedAircraft = UNDEFINED;
    bool airliner = false;

    bool apuMaster = false;
    bool apuStart = false;
    int apuStartFlash = 0;
    bool apuBleed = false;
    int flapsVal = INT_MIN;
    int flapsUpVal = 0;
    int flapsDownVal = 20;
    int lastFlapsPos = -1;      // 0 = up, 4 = full
    bool seenBrakeOff = false;

    // Hardware controls
    int battery1Control = -1;
    int battery2Control = -1;
    int fuelPumpControl = -1;
    int beaconControl = -1;
    int landControl = -1;
    int taxiControl = -1;
    int navControl = -1;
    int strobeControl = -1;
    int pitotHeatControl = -1;
    int avionics1Control = -1;
    int avionics2Control = -1;
    int apuMasterControl = -1;
    int apuStartControl = -1;
    int apuBleedControl = -1;
    int flapsUpControl = -1;
    int flapsPosControl = -1;
    int flapsDownControl = -1;
    int parkBrakeOffControl = -1;
    int parkBrakeOnControl = -1;

    int prevBattery1Toggle = -1;
    int prevBattery2Toggle = -1;
    int prevFuelPumpToggle = -1;
    int prevBeaconToggle = -1;
    int prevLandToggle = -1;
    int prevTaxiToggle = -1;
    int prevNavToggle = -1;
    int prevStrobeToggle = -1;
    int prevPitotHeatToggle = -1;
    int prevAvionics1Toggle = -1;
    int prevAvionics2Toggle = -1;
    int prevApuMasterPush = 0;
    int prevApuStartPush = 0;
    int prevApuBleedPush = 0;
    int prevFlapsUpToggle = -1;
    int prevFlapsDownToggle = -1;
    int prevParkBrakeOffToggle = -1;
    int prevParkBrakeOnToggle = -1;

    time_t lastApuMasterAdjust = 0;
    time_t lastApuStartAdjust = 0;
    time_t lastApuBleedAdjust = 0;
    time_t now;

public:
    powerLights();
    void render();
    void update();

private:
    void addGpio();
    void gpioSwitchesInput();
    void gpioButtonsInput();
    void gpioFlapsInput();
    void gpioParkBrakeInput();
};

#endif // _POWER_LIGHTS_H
